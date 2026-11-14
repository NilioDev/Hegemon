// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/production.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/empire.hpp"

#include <algorithm>
#include <stdexcept>

namespace hegemon {

namespace {

const std::vector<std::string>& rowsOf(const Parameters& p, std::string_view setting) {
    const ParameterDefinition* d = p.definition(setting);
    if (d == nullptr || d->rowNames.empty()) {
        throw std::out_of_range("missing setting: " + std::string(setting));
    }
    return d->rowNames[0];
}

bool knows(const Parameters& p, const Player& player, std::string_view tech) {
    return player.knowledge.knowsTech(static_cast<int>(p.integer("tech", {tech}, "id")));
}

// Measured: a colony base needs a planet of the system nobody lives on
// (assumed: asteroids and gas giants don't count, as only an outpost or an
// artificial planet settles them).
bool freePlanet(const Game& game, const Colony& colony) {
    const Planet& home = game.galaxy.planets.at(static_cast<std::size_t>(colony.planet));
    const Star& star = game.galaxy.stars.at(static_cast<std::size_t>(home.star));
    for (const int index : star.orbits) {
        if (index < 0 || static_cast<std::size_t>(index) >= game.galaxy.planets.size() || index == colony.planet) {
            continue;
        }
        if (game.galaxy.planets[static_cast<std::size_t>(index)].type != SatelliteType::Planet) {
            continue;
        }
        const bool settled = std::any_of(game.colonies.begin(), game.colonies.end(),
                                         [&](const Colony& c) { return c.planet == index; });
        if (!settled) {
            return true;
        }
    }
    return false;
}

} // namespace

Buildable buildable(const Parameters& p, const Game& game, std::size_t colonyIndex) {
    const Colony& colony = game.colonies.at(colonyIndex);
    const Player& player = game.players.at(static_cast<std::size_t>(colony.player));
    Buildable b;
    b.buildings.emplace_back(kTradeGoods);
    if (p.integer("/nohousing") == 0) {
        b.buildings.emplace_back(kHousing);
    }
    for (const std::string& name : rowsOf(p, "building")) {
        if (name == kColonyBase || colony.has(name)) {
            continue;
        }
        const auto tech = static_cast<int>(p.integer("building", {name}, "tech_id"));
        if (player.knowledge.knowsTech(tech)) {
            b.buildings.push_back(name);
        }
    }
    std::stable_sort(b.buildings.begin(), b.buildings.end(), [&](const std::string& x, const std::string& y) {
        return productOrder(p, x) < productOrder(p, y);
    });
    if (knows(p, player, kColonyBase) && freePlanet(game, colony)) {
        b.others.emplace_back(kColonyBase);
    }
    if (knows(p, player, kFreighters)) {
        b.others.emplace_back(kFreighters);
    }
    if (knows(p, player, kColonyShip)) {
        b.others.emplace_back(kColonyShip);
    }
    if (knows(p, player, kOutpostShip)) {
        b.others.emplace_back(kOutpostShip);
    }
    if (knows(p, player, "transport")) {
        b.others.emplace_back(kTransportShip);
    }
    b.others.emplace_back(kSpy);
    return b;
}

int productCost(const Parameters& p, std::string_view product) {
    if (product == kTradeGoods || product == kHousing) {
        return 0;
    }
    if (product == kColonyShip || product == kOutpostShip || product == kTransportShip) {
        return static_cast<int>(p.integer("lander_ship_cost", {}, product));
    }
    if (product == kFreighters || product == kSpy) {
        return static_cast<int>(p.integer("misc_costs", {}, product));
    }
    return static_cast<int>(p.integer("building", {product}, "cost"));
}

int productOrder(const Parameters& p, std::string_view product) {
    return static_cast<int>(p.integer("prod_format", {product}, "order"));
}

bool repeatable(std::string_view product) {
    return product == kTradeGoods || product == kHousing || product == kFreighters || product == kColonyShip ||
           product == kOutpostShip || product == kTransportShip || product == kSpy;
}

bool addToBuildList(Colony& colony, std::string_view product) {
    if (colony.buildList.size() >= Colony::kBuildListLength) {
        return false;
    }
    if (!repeatable(product) &&
        std::find(colony.buildList.begin(), colony.buildList.end(), product) != colony.buildList.end()) {
        return false;
    }
    colony.buildList.emplace_back(product);
    return true;
}

int buyPrice(int cost, int produced) {
    // Measured: each point still to make costs 10 BC in the first tenth of
    // the product, 5 up to its half and 2 after (assumed: the tenth and the
    // half rounded down). With nothing made that is four BC a point.
    const int tenth = cost / 10, half = cost / 2;
    const int made = std::clamp(produced, 0, cost);
    return 10 * std::max(0, tenth - made) + 5 * std::max(0, half - std::max(made, tenth)) +
           2 * (cost - std::max(made, half));
}

Purchase purchase(const Parameters& parameters, const Colony& colony, int money, int perTurn) {
    Purchase p;
    if (colony.boughtPrice > 0) {
        p.kind = Purchase::Kind::Undo;
        p.price = colony.boughtPrice;
        p.amount = colony.boughtProduction;
        return p;
    }
    if (colony.buildList.empty()) {
        p.kind = Purchase::Kind::NothingSelected;
        return p;
    }
    const std::string& product = colony.buildList.front();
    if (product == kHousing || product == kTradeGoods) {
        p.kind = Purchase::Kind::NotBuyable;
        return p;
    }
    const int cost = productCost(parameters, product);
    p.amount = std::max(0, cost - colony.produced);
    if (p.amount <= perTurn) {
        p.kind = Purchase::Kind::ThisTurn;
        return p;
    }
    p.price = buyPrice(cost, colony.produced);
    if (p.price > money) {
        p.kind = Purchase::Kind::Short;
        p.shortBy = p.price - money;
    } else {
        p.kind = Purchase::Kind::Offer;
    }
    return p;
}

void buy(Colony& colony, int& money, const Purchase& p) {
    if (p.kind == Purchase::Kind::Offer) {
        money -= p.price;
        colony.produced += p.amount;
        colony.boughtPrice = p.price;
        colony.boughtProduction = p.amount;
    } else if (p.kind == Purchase::Kind::Undo) {
        money += colony.boughtPrice;
        colony.produced -= colony.boughtProduction;
        colony.boughtPrice = 0;
        colony.boughtProduction = 0;
    }
}

} // namespace hegemon
