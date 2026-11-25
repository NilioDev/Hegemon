// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/computer_players.hpp"

#include "hegemon/rules/fleet.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/tech.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

namespace hegemon {

namespace {

constexpr std::size_t kCategories = 8;

bool hostile(Climate c) {
    return c == Climate::Toxic || c == Climate::Radiated || c == Climate::Barren;
}

std::vector<int> colonyStars(const Game& game, int player) {
    std::vector<int> stars;
    for (const Colony& c : game.colonies) {
        if (c.player == player) {
            stars.push_back(game.galaxy.planets.at(static_cast<std::size_t>(c.planet)).star);
        }
    }
    return stars;
}

bool contains(const std::vector<int>& v, int x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

// Seen in the original: when a project is done, the cheapest field it can
// research; between fields of the same cost one at random (assumed).
void chooseResearch(Player& player, const TechTree& tree, Random& random) {
    std::vector<int> cheapest;
    int cost = std::numeric_limits<int>::max();
    for (std::size_t c = 0; c < kCategories; ++c) {
        const int field = tree.researchable(player.knowledge, static_cast<TechCategory>(c));
        if (field <= 0) {
            continue;
        }
        const int fieldCost = tree.field(field).cost;
        if (fieldCost < cost) {
            cost = fieldCost;
            cheapest.clear();
        }
        if (fieldCost == cost) {
            cheapest.push_back(field);
        }
    }
    if (cheapest.empty()) {
        return;
    }
    player.research = cheapest[random.below(static_cast<std::uint32_t>(cheapest.size()))];
    player.researched = 0;
    player.researchTech = 0;
}

// The planet a colony ship of `player` goes to settle: at the nearest star
// with one, but none of the player's own stars (seen: the home systems'
// planets were left), the planets of a climate one can farm before the
// others, then the one that holds the most people (seen on three races).
std::optional<int> colonyTarget(const Game& game, const Economy& economy, int player, std::size_t shipIndex) {
    const Ship& ship = game.ships[shipIndex];
    const Player& owner = game.players.at(static_cast<std::size_t>(player));
    const std::vector<int> own = colonyStars(game, player);
    const MapPoint from = shipPlace(game, ship);
    std::optional<int> best;
    int bestDistance = 0;
    for (std::size_t s = 0; s < game.galaxy.stars.size(); ++s) {
        const int star = static_cast<int>(s);
        if (contains(own, star)) {
            continue;
        }
        std::optional<int> pick;
        for (const int planet : colonySites(game, star)) {
            const bool taken = std::any_of(game.ships.begin(), game.ships.end(), [&](const Ship& other) {
                return &other != &ship && other.player == player && other.landOn == planet;
            });
            const Planet& p = game.galaxy.planets[static_cast<std::size_t>(planet)];
            if (taken || economy.maxPopulation(owner, p) <= 0) {
                continue;
            }
            if (!pick) {
                pick = planet;
                continue;
            }
            const Planet& q = game.galaxy.planets[static_cast<std::size_t>(*pick)];
            const bool better = hostile(q.climate) != hostile(p.climate)
                                    ? !hostile(p.climate)
                                    : economy.maxPopulation(owner, p) > economy.maxPopulation(owner, q);
            if (better) {
                pick = planet;
            }
        }
        if (!pick) {
            continue;
        }
        const Star& at = game.galaxy.stars[s];
        const int d = mapDistance(from, {at.x, at.y});
        if (!best || d < bestDistance) {
            best = pick;
            bestDistance = d;
        }
    }
    return best;
}

// Seen in the original: a scout goes out to a star nobody of its player
// has seen or is going to, then back to a colony, and out again.
std::optional<int> scoutTarget(const Game& game, int player, std::size_t shipIndex) {
    const Ship& ship = game.ships[shipIndex];
    const Player& owner = game.players.at(static_cast<std::size_t>(player));
    const std::vector<int> own = colonyStars(game, player);
    const MapPoint from = shipPlace(game, ship);
    const bool home = contains(own, ship.star);
    std::optional<int> best;
    int bestDistance = 0;
    for (std::size_t s = 0; s < game.galaxy.stars.size(); ++s) {
        const int star = static_cast<int>(s);
        if (home) {
            const bool going = std::any_of(game.ships.begin(), game.ships.end(), [&](const Ship& other) {
                return other.player == player && other.destination == star;
            });
            if (explored(owner, star) || going || game.galaxy.stars[s].colour == StarColour::BlackHole) {
                continue;
            }
        } else if (!contains(own, star)) {
            continue;
        }
        const Star& at = game.galaxy.stars[s];
        const int d = mapDistance(from, {at.x, at.y});
        if (!best || d < bestDistance) {
            best = star;
            bestDistance = d;
        }
    }
    return best;
}

} // namespace

void orderComputerPlayers(Game& game, const Parameters& parameters, const TechTree& tree, Random& random) {
    const Economy economy(parameters);
    for (std::size_t i = 0; i < game.players.size(); ++i) {
        Player& player = game.players[i];
        const int id = static_cast<int>(i);
        if (player.human) {
            continue;
        }
        if (player.research == 0) {
            chooseResearch(player, tree, random);
        }
        // Seen: the home colonies make trade goods, new colonies of one
        // person housing (the original also had some build marine
        // barracks or ships; not copied yet).
        for (Colony& c : game.colonies) {
            if (c.player != id) {
                continue;
            }
            if (c.buildList.empty()) {
                c.buildList.push_back(std::string(c.population() <= 1 ? kHousing : kTradeGoods));
            }
            // Seen: the Meklar's home of 7 farmers and a worker was put to
            // 2 farmers, 2 workers and 4 scientists the next turn; colonies
            // with scientists were left alone. Assumed: a colony of two or
            // more without scientists is set as a new colony would be.
            if (c.population() >= 2 && c.jobs[static_cast<std::size_t>(Job::Scientist)] == 0) {
                c.jobs = economy.startingJobs(player, game.galaxy.planets.at(static_cast<std::size_t>(c.planet)),
                                              c.population());
            }
        }
        // Seen: the scouts went elsewhere than the colony ships, so these
        // choose first.
        for (std::size_t s = 0; s < game.ships.size(); ++s) {
            Ship& ship = game.ships[s];
            if (ship.player != id || ship.star < 0 || ship.destination >= 0) {
                continue;
            }
            if (ship.kind == ShipKind::ColonyShip) {
                const bool there = ship.landOn >= 0 &&
                                   game.galaxy.planets.at(static_cast<std::size_t>(ship.landOn)).star == ship.star;
                if (there) {
                    continue; // lands after the ships moved
                }
                if (const auto planet = colonyTarget(game, economy, id, s)) {
                    ship.landOn = *planet;
                    const std::size_t one[] = {s};
                    orderShips(game, parameters, one, game.galaxy.planets.at(static_cast<std::size_t>(*planet)).star);
                }
            }
        }
        for (std::size_t s = 0; s < game.ships.size(); ++s) {
            const Ship& ship = game.ships[s];
            if (ship.player != id || ship.star < 0 || ship.destination >= 0 || ship.kind != ShipKind::Scout) {
                continue;
            }
            if (const auto star = scoutTarget(game, id, s)) {
                const std::size_t one[] = {s};
                orderShips(game, parameters, one, *star);
            }
        }
    }
}

void landComputerColonyShips(Game& game, const Parameters& parameters) {
    (void)parameters;
    for (std::size_t s = game.ships.size(); s-- > 0;) {
        const Ship& ship = game.ships[s];
        if (ship.kind != ShipKind::ColonyShip || ship.landOn < 0 || ship.star < 0 ||
            static_cast<std::size_t>(ship.player) >= game.players.size() ||
            game.players[static_cast<std::size_t>(ship.player)].human) {
            continue;
        }
        const int planet = ship.landOn;
        if (game.galaxy.planets.at(static_cast<std::size_t>(planet)).star != ship.star) {
            continue;
        }
        const std::vector<int> sites = colonySites(game, ship.star);
        if (contains(sites, planet)) {
            foundColony(game, s, planet);
        } else {
            game.ships[s].landOn = -1; // taken: it looks again next turn
        }
    }
}

} // namespace hegemon
