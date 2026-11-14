// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/empire.hpp"

#include "hegemon/rules/fleet.hpp"
#include "hegemon/rules/production.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/random.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace hegemon {

namespace {

constexpr std::array<std::string_view, kClimates> kClimateNames = {
    "toxic", "radiated", "barren", "desert", "tundra", "ocean", "swamp", "arid", "terran", "gaia"};
constexpr std::array<std::string_view, 5> kMineralNames = {"ultra_poor", "poor", "abundant", "rich", "ultra_rich"};
constexpr std::array<std::string_view, 5> kSizeNames = {"tiny", "small", "medium", "large", "huge"};

// The government pick's numbers.
constexpr int kFeudal = 0;
constexpr int kDemocracy = 4;
constexpr int kUnification = 6;
// govt_bonus counts in fives of a percent.
constexpr int kGovernmentStep = 5;

// Every pop pays one BC in taxes (measured), two halves.
constexpr int kTax2 = 2;
// Measured: scientists are put to work until they make 3/5 of the base
// research of everyone who doesn't farm (see docs/rules/start.md).
constexpr int kScienceShareNum = 3, kScienceShareDen = 5;

// Thousands of people grown that make a new person.
constexpr int kGrown = 1000;

// The buildings each home colony starts with (average technology).
constexpr std::array<std::string_view, 2> kHomeBuildings = {"star_base", "marine_barracks"};

int government(const Player& player) { return player.picks[RacePicks::Government]; }

// A field's cost in research points, by its id.
int cost(const Parameters& p, int field) {
    const ParameterDefinition* d = p.definition("tech_field");
    if (d == nullptr || d->rowNames.empty()) {
        return 0;
    }
    for (std::size_t r = 0; r < d->rowNames[0].size(); ++r) {
        if (p.integer("tech_field", {d->rowNames[0][r]}, "id") == field) {
            return static_cast<int>(p.integer("tech_field", {d->rowNames[0][r]}, "cost"));
        }
    }
    return 0;
}

} // namespace

bool Colony::has(std::string_view building) const {
    return std::find(buildings.begin(), buildings.end(), building) != buildings.end();
}

Economy::Economy(const Parameters& parameters) : p_(parameters) {
    // Fail early when a setting is missing.
    (void)p_.integer("food_farmer_base", {}, "terran");
    (void)p_.integer("production_worker_base", {}, "abundant");
    (void)p_.integer("research_scientist_bonus", {}, "base_scientist");
}

int Economy::pickValue(const RacePicks& picks, RacePicks::Pick pick, std::string_view name) const {
    const int level = picks[pick];
    if (level < 1 || level > 3) {
        return 0;
    }
    return static_cast<int>(p_.integer("race_pick", {}, std::string(name) + std::to_string(level) + "_value"));
}

int Economy::farmerFood2(const Player& player, const Planet& planet) const {
    auto climate = planet.climate;
    // Measured on the Trilarians' ocean home world: an aquatic race farms
    // an ocean world as well as a gaia one.
    if (player.picks[RacePicks::Aquatic] != 0 && climate == Climate::Ocean) {
        climate = Climate::Gaia;
    }
    const int base = static_cast<int>(
        p_.integer("food_farmer_base", {}, kClimateNames[static_cast<std::size_t>(climate)]));
    return std::max(0, base + pickValue(player.picks, RacePicks::Farming, "farming"));
}

int Economy::foodTotal2(const Player& player, const Planet& planet, int farmers) const {
    int bonus = 0;
    if (government(player) == kUnification) {
        bonus = static_cast<int>(p_.integer("govt_bonus", {}, "unification_food")) * kGovernmentStep;
    }
    return farmers * farmerFood2(player, planet) * (100 + bonus) / 100;
}

int Economy::eaten2(const Player& player, int population) const {
    if (player.picks[RacePicks::Lithovore] != 0) {
        return 0;
    }
    return player.picks[RacePicks::Cybernetic] != 0 ? population : 2 * population;
}

int Economy::production(const Player& player, const Planet& planet, int workers, int* pollution) const {
    const int perWorker =
        static_cast<int>(p_.integer("production_worker_base", {}, kMineralNames[static_cast<std::size_t>(planet.minerals)])) +
        pickValue(player.picks, RacePicks::Industry, "industry");
    int made = workers * std::max(0, perWorker);
    if (government(player) == kUnification) {
        made = made * (100 + static_cast<int>(p_.integer("govt_bonus", {}, "unification_production")) * kGovernmentStep) / 100;
    }
    if (player.picks[RacePicks::Tolerant] == 0) {
        // Half of what's made beyond what the planet takes, rounded up, goes
        // to cleaning up the pollution.
        const int exempt =
            static_cast<int>(p_.integer("planet_no_pollution_prod", {}, kSizeNames[static_cast<std::size_t>(planet.size)]));
        if (made > exempt) {
            const int lost = (made - exempt + 1) / 2;
            made -= lost;
            if (pollution != nullptr) {
                *pollution = lost;
            }
        }
    }
    return made;
}

int Economy::scientistResearch100(const Player& player, const Planet& planet) const {
    int each = static_cast<int>(p_.integer("research_scientist_bonus", {}, "base_scientist")) +
               pickValue(player.picks, RacePicks::Science, "science");
    if (planet.special == Special::AncientArtifacts) {
        each += static_cast<int>(p_.integer("research_scientist_bonus", {}, "artifacts_planet"));
    }
    int bonus = 0;
    if (government(player) == kFeudal) {
        bonus = static_cast<int>(p_.integer("govt_bonus", {}, "feudal_science")) * kGovernmentStep;
    } else if (government(player) == kDemocracy) {
        bonus = static_cast<int>(p_.integer("govt_bonus", {}, "democracy_science")) * kGovernmentStep;
    }
    return std::max(0, each) * (100 + bonus);
}

int Economy::maxPopulation(const Player& player, const Planet& planet) const {
    auto climate = planet.climate;
    // Measured on the Trilarians' home world, as for farming.
    if (player.picks[RacePicks::Aquatic] != 0 && climate == Climate::Ocean) {
        climate = Climate::Gaia;
    }
    const auto size = kSizeNames[static_cast<std::size_t>(planet.size)];
    int most = static_cast<int>(p_.integer("pop_max", {}, size) *
                                p_.integer("pop_climate", {}, kClimateNames[static_cast<std::size_t>(climate)]) / 100);
    if (player.picks[RacePicks::Tolerant] != 0) {
        most = most * (100 + static_cast<int>(p_.integer("pop_bonus_tolerant"))) / 100;
    }
    if (player.picks[RacePicks::Subterranean] != 0) {
        most += static_cast<int>(p_.integer("pop_bonus_subterranean", {}, size));
    }
    return most;
}

int Economy::growth(const Player& player, const Planet& planet, int population, int housing) const {
    const int most = maxPopulation(player, planet);
    if (population <= 0 || most <= population) {
        return 0;
    }
    // The fan patch's formula: the square root of FACTOR × people × room
    // left ÷ room, rounded down; then the race's growth pick.
    const std::int64_t factor = p_.integer("growth_formula_factor");
    const auto base = static_cast<int>(
        std::sqrt(static_cast<double>(factor * population * (most - population) / most)));
    int bonus = static_cast<int>(p_.integer("growth_rate_bonus", {}, "general_modifier")) +
                pickValue(player.picks, RacePicks::Growth, "growth");
    if (housing > 0) {
        // The fan patch's manual: FACTOR × production ÷ people, as a bonus
        // in percent.
        const auto factor2 = static_cast<int>(p_.integer("housing_formula_factor"));
        bonus += p_.integer("housing_no_popagg") != 0 ? factor2 * housing : factor2 * housing / population;
    }
    return std::max(0, base * (100 + bonus) / 100);
}

int Economy::growth(const Game& game, const Colony& colony) const {
    const Player& player = game.players.at(static_cast<std::size_t>(colony.player));
    const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(colony.planet));
    int housing = 0;
    if (!colony.buildList.empty() && colony.buildList.front() == "housing") {
        const ColonyOutput out = output(game, colony);
        housing = std::max(0, out.production - out.upkeep);
    }
    return growth(player, planet, colony.population(), housing);
}

std::array<int, kJobs> Economy::startingJobs(const Player& player, const Planet& planet, int population) const {
    std::array<int, kJobs> jobs{};
    if (population <= 0) {
        return jobs;
    }
    const int need = eaten2(player, population);
    int farmers = 0;
    while (farmers < population && foodTotal2(player, planet, farmers) < need) {
        ++farmers;
    }
    const int rest = population - farmers;
    int scientists = 0;
    if (rest >= 2) {
        const int perScientist = scientistResearch100(player, planet);
        const int target =
            rest * static_cast<int>(p_.integer("research_scientist_bonus", {}, "base_scientist")) * 100 * kScienceShareNum;
        scientists = 1;
        while (scientists < rest - 1 && scientists * perScientist * kScienceShareDen < target) {
            ++scientists;
        }
    }
    jobs[static_cast<std::size_t>(Job::Farmer)] = farmers;
    jobs[static_cast<std::size_t>(Job::Scientist)] = scientists;
    jobs[static_cast<std::size_t>(Job::Worker)] = rest - scientists;
    return jobs;
}

ColonyOutput Economy::output(const Game& game, const Colony& colony) const {
    const Player& player = game.players.at(static_cast<std::size_t>(colony.player));
    const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(colony.planet));
    ColonyOutput out;
    const int population = colony.population();
    out.food2 = foodTotal2(player, planet, colony.jobs[static_cast<std::size_t>(Job::Farmer)]);
    out.eaten2 = eaten2(player, population);
    out.production = production(player, planet, colony.jobs[static_cast<std::size_t>(Job::Worker)], &out.pollution);
    out.upkeep = player.picks[RacePicks::Cybernetic] != 0 ? population / 2 : 0;
    // Rounded to the nearest point (measured: three Elerian scientists under
    // feudalism, 4.5, make 5).
    out.research = (colony.jobs[static_cast<std::size_t>(Job::Scientist)] * scientistResearch100(player, planet) + 50) / 100;

    int taxEach2 = kTax2 + pickValue(player.picks, RacePicks::Money, "money");
    int tax2 = population * taxEach2;
    if (government(player) == kDemocracy) {
        tax2 = tax2 * (100 + static_cast<int>(p_.integer("govt_bonus", {}, "democracy_money")) * kGovernmentStep) / 100;
    }
    // Measured: with nothing to build, or trade goods first in the build
    // list, what's made goes to trade goods.
    int trade2 = 0;
    if (colony.buildList.empty() || colony.buildList.front() == "trade_goods") {
        const int ratio = static_cast<int>(p_.integer(
            "trade_goods_ratio", {}, player.picks[RacePicks::FantasticTraders] != 0 ? "fantastic_traders" : "normal"));
        trade2 = std::max(0, out.production - out.upkeep) * 2 * ratio / 100;
    }
    int upkeep2 = 0;
    for (const auto& b : colony.buildings) {
        upkeep2 += 2 * static_cast<int>(p_.integer("building", {b}, "maintenance"));
    }
    out.maintenance2 = upkeep2;
    out.income2 = tax2 + trade2 - upkeep2;
    return out;
}

EmpireSummary Economy::summary(const Game& game, int player) const {
    EmpireSummary s;
    const Player& who = game.players.at(static_cast<std::size_t>(player));
    s.money = who.money;
    int income2 = 0, food2 = 0;
    s.commandPoints = static_cast<int>(p_.integer("command_points", {}, "starting"));
    for (const Colony& c : game.colonies) {
        if (c.player != player) {
            continue;
        }
        const ColonyOutput o = output(game, c);
        income2 += o.income2;
        food2 += o.food2 - o.eaten2;
        s.research += o.research;
        for (const std::string_view base : {"star_base", "battlestation", "star_fortress"}) {
            if (c.has(base)) {
                s.commandPoints += static_cast<int>(p_.integer("command_points", {}, base));
            }
        }
        if (who.picks[RacePicks::Warlord] != 0) {
            s.commandPoints += static_cast<int>(p_.integer("command_points", {}, "warlord_planet"));
        }
    }
    // Scouts, colony ships and outpost ships are frigates.
    for (const Ship& ship : game.ships) {
        if (ship.player == player) {
            s.commandUsed += static_cast<int>(p_.integer("ship_command_points_usage", {}, "frigate"));
        }
    }
    // Halves round up (measured: +6.5 BC shows as +7).
    s.income = income2 >= 0 ? (income2 + 1) / 2 : -((-income2) / 2);
    s.food = food2 >= 0 ? food2 / 2 : -((-food2 + 1) / 2);
    s.researchField = who.research;
    if (who.research != 0) {
        s.researchTurns = researchTurns(cost(p_, who.research), who.researched, s.research);
    }
    return s;
}

int researchTurns(int cost, int researched, int perTurn) {
    const int left = cost * 3 / 2 - researched;
    if (perTurn <= 0 || left <= 0) {
        return 1;
    }
    return (left + perTurn - 1) / perTurn;
}

std::string planetName(const Galaxy& galaxy, int planet, const std::string& prime) {
    static constexpr std::array<std::string_view, kOrbits> kRoman = {"I", "II", "III", "IV", "V"};
    const Planet& p = galaxy.planets.at(static_cast<std::size_t>(planet));
    const std::string& star = galaxy.stars.at(static_cast<std::size_t>(p.star)).name;
    // Measured: Sal's first planet, in its second orbit, is Sal Prime.
    std::size_t before = 0;
    for (const Planet& other : galaxy.planets) {
        if (other.star == p.star && other.type == SatelliteType::Planet && other.orbit < p.orbit) {
            ++before;
        }
    }
    if (before == 0 || before >= kOrbits) {
        return star + " " + prime;
    }
    return star + " " + std::string(kRoman[before]);
}

namespace {

// The colony's production goes into the first of its build list; what it
// finishes leaves the list (assumed: what is left over is kept for the
// next).
void produce(Game& game, const Parameters& parameters, const Economy& economy, Colony& c) {
    if (c.buildList.empty()) {
        return;
    }
    const std::string product = c.buildList.front();
    if (product == kTradeGoods || product == kHousing) {
        return; // they stay until taken out, and their effect is in the income and growth
    }
    const ColonyOutput out = economy.output(game, c);
    c.produced += std::max(0, out.production - out.upkeep);
    Player& player = game.players.at(static_cast<std::size_t>(c.player));
    const int cost = productCost(parameters, product, player.designs);
    if (c.produced < cost) {
        return;
    }
    c.produced -= cost;
    c.buildList.erase(c.buildList.begin());
    const int star = game.galaxy.planets.at(static_cast<std::size_t>(c.planet)).star;
    if (product == kColonyShip) {
        game.ships.push_back({c.player, ShipKind::ColonyShip, star});
    } else if (product == kOutpostShip) {
        game.ships.push_back({c.player, ShipKind::OutpostShip, star});
    } else if (product == kTransportShip) {
        game.ships.push_back({c.player, ShipKind::Transport, star});
    } else if (product == kFreighters) {
        ++player.freighterFleets;
    } else if (product == kSpy) {
        ++player.spies;
    } else if (const auto slot = designSlot(product)) {
        game.ships.push_back({c.player, ShipKind::Combat, star, static_cast<int>(*slot)});
    } else if (product == kColonyBase) {
        // A new colony of one on the system's first free planet (assumed).
        const Star& system = game.galaxy.stars.at(static_cast<std::size_t>(star));
        for (const int index : system.orbits) {
            if (index < 0 || static_cast<std::size_t>(index) >= game.galaxy.planets.size()) {
                continue;
            }
            const Planet& planet = game.galaxy.planets[static_cast<std::size_t>(index)];
            const bool settled = std::any_of(game.colonies.begin(), game.colonies.end(),
                                             [&](const Colony& o) { return o.planet == index; });
            if (planet.type == SatelliteType::Planet && !settled) {
                Colony colony;
                colony.player = c.player;
                colony.planet = index;
                colony.jobs = economy.startingJobs(player, planet, 1);
                game.colonies.push_back(std::move(colony));
                break;
            }
        }
    } else {
        c.buildings.push_back(product);
    }
}

} // namespace

void endTurn(Game& game, const Parameters& parameters, const TechTree& tree, Random& random) {
    const Economy economy(parameters);
    const bool fixedCost = parameters.integer("fixed_research_cost") != 0;
    const bool keep = parameters.integer("keep_research_on_breakthrough") != 0;
    ++game.stardate;
    // The income and research are those of the turn that ends; the people
    // grow after (assumed).
    std::vector<EmpireSummary> summaries;
    for (std::size_t i = 0; i < game.players.size(); ++i) {
        summaries.push_back(economy.summary(game, static_cast<int>(i)));
    }
    // The growth is that of the turn that ends too, with its housing.
    std::vector<int> grown;
    for (const Colony& c : game.colonies) {
        grown.push_back(economy.growth(game, c));
    }
    // Measured: ships sent off this turn go a turn's way; those built now
    // stay at their colony's star (assumed).
    moveShips(game);
    // What is built is made by the people before they grow (assumed), by
    // index: a colony base adds a colony.
    for (std::size_t i = 0, n = game.colonies.size(); i < n; ++i) {
        produce(game, parameters, economy, game.colonies[i]);
        game.colonies[i].boughtProduction = 0;
        game.colonies[i].boughtPrice = 0;
    }
    for (std::size_t i = 0; i < grown.size(); ++i) {
        Colony& c = game.colonies[i];
        const Player& player = game.players.at(static_cast<std::size_t>(c.player));
        const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
        c.growth += grown[i];
        // Measured: a thousand grown makes a new person. Assumed: who
        // works, and that nothing is kept when the planet is full.
        while (c.growth >= kGrown) {
            c.growth -= kGrown;
            if (c.population() < economy.maxPopulation(player, planet)) {
                ++c.jobs[static_cast<std::size_t>(Job::Worker)];
            } else {
                c.growth = 0;
            }
        }
    }
    for (std::size_t i = 0; i < game.players.size(); ++i) {
        const EmpireSummary& s = summaries[i];
        Player& player = game.players[i];
        player.learntField = 0;
        player.learntTechs.clear();
        player.money = std::max(0, player.money + s.income);
        if (player.research == 0) {
            continue;
        }
        player.researched += s.research;
        const TechField& field = tree.field(player.research);
        // A breakthrough is certain at twice the cost; from the cost on
        // it may come early (the chance is assumed to grow evenly).
        bool done = player.researched >= 2 * field.cost;
        if (!done && !fixedCost && player.researched >= field.cost && field.cost > 0) {
            done = static_cast<int>(random.below(static_cast<std::uint32_t>(field.cost))) <
                   player.researched - field.cost;
        }
        if (!done) {
            continue;
        }
        // A creative race learns all the field's applications, the others
        // the one chosen (measured: Select New Research takes the one
        // clicked).
        std::vector<int> techs(field.techs.begin(), field.techs.end());
        if (player.picks[RacePicks::Creative] == 0 && !techs.empty()) {
            const auto chosen = std::find(techs.begin(), techs.end(), player.researchTech);
            techs = {chosen != techs.end() ? *chosen : techs.front()};
        }
        tree.learn(player.knowledge, field.id, techs);
        player.learntField = field.id;
        player.learntTechs = techs;
        player.researched = keep ? std::max(0, player.researched - 2 * field.cost) : 0;
        player.researchTech = 0;
        player.research = 0;
    }
}

Game startGame(const Parameters& parameters, Galaxy galaxy, const std::vector<RacePicks>& picks,
               const std::vector<std::size_t>& banners, std::uint64_t seed) {
    if (picks.size() != galaxy.homes.size() || banners.size() != galaxy.homes.size()) {
        throw std::invalid_argument("startGame: one race and banner per home system");
    }
    const Economy economy(parameters);
    const TechTree tree(parameters);
    // Settings without the ship tables (as in some tests) give no designs.
    std::optional<ShipParts> parts;
    if (parameters.definition("weapon") != nullptr) {
        parts.emplace(parameters, tree);
    }
    Random random(seed);
    Game game;
    game.galaxy = std::move(galaxy);
    const int population = static_cast<int>(parameters.integer("starting_condition", {}, "starting_population"));
    const int money = static_cast<int>(parameters.integer("starting_condition", {}, "starting_money"));
    const bool noShips = parameters.integer("no_starting_ships") != 0;
    const bool outpost = parameters.integer("starting_outpost_ship") != 0;
    for (std::size_t i = 0; i < game.galaxy.homes.size(); ++i) {
        const HomeSystem& home = game.galaxy.homes[i];
        Player player;
        player.race = home.race;
        player.picks = picks[i];
        player.banner = banners[i];
        player.human = i == 0;
        player.money = money;
        player.knowledge = tree.averageStart();
        player.explored.assign(game.galaxy.stars.size(), false);
        if (home.star >= 0 && static_cast<std::size_t>(home.star) < game.galaxy.stars.size()) {
            player.explored[static_cast<std::size_t>(home.star)] = true;
        }
        if (parts) {
            player.designs = startingDesigns(parameters, *parts, player.knowledge, random);
        }
        game.players.push_back(player);

        Colony colony;
        colony.player = static_cast<int>(i);
        colony.planet = home.planet;
        colony.jobs = economy.startingJobs(player, game.galaxy.planets.at(static_cast<std::size_t>(home.planet)), population);
        // Measured: a Unification has no capitol.
        if (player.picks[RacePicks::Government] != kUnification) {
            colony.buildings.emplace_back("capitol");
        }
        for (const auto b : kHomeBuildings) {
            colony.buildings.emplace_back(b);
        }
        game.colonies.push_back(std::move(colony));

        if (!noShips) {
            const int player_ = static_cast<int>(i);
            game.ships.push_back({player_, ShipKind::Scout, home.star});
            game.ships.push_back({player_, ShipKind::Scout, home.star});
            game.ships.push_back({player_, ShipKind::ColonyShip, home.star});
            if (outpost) {
                game.ships.push_back({player_, ShipKind::OutpostShip, home.star});
            }
        }
    }
    return game;
}

} // namespace hegemon
