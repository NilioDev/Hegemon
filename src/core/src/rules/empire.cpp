// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/empire.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/random.hpp"

#include <algorithm>
#include <cmath>
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

int Economy::production(const Player& player, const Planet& planet, int workers) const {
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
            made -= (made - exempt + 1) / 2;
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

int Economy::growth(const Player& player, const Planet& planet, int population) const {
    const int most = maxPopulation(player, planet);
    if (population <= 0 || most <= population) {
        return 0;
    }
    // The fan patch's formula: the square root of FACTOR × people × room
    // left ÷ room, rounded down; then the race's growth pick.
    const std::int64_t factor = p_.integer("growth_formula_factor");
    const auto base = static_cast<int>(
        std::sqrt(static_cast<double>(factor * population * (most - population) / most)));
    const int bonus = static_cast<int>(p_.integer("growth_rate_bonus", {}, "general_modifier")) +
                      pickValue(player.picks, RacePicks::Growth, "growth");
    return std::max(0, base * (100 + bonus) / 100);
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
    out.production = production(player, planet, colony.jobs[static_cast<std::size_t>(Job::Worker)]);
    out.upkeep = player.picks[RacePicks::Cybernetic] != 0 ? population / 2 : 0;
    out.research = colony.jobs[static_cast<std::size_t>(Job::Scientist)] * scientistResearch100(player, planet) / 100;

    int taxEach2 = kTax2 + pickValue(player.picks, RacePicks::Money, "money");
    int tax2 = population * taxEach2;
    if (government(player) == kDemocracy) {
        tax2 = tax2 * (100 + static_cast<int>(p_.integer("govt_bonus", {}, "democracy_money")) * kGovernmentStep) / 100;
    }
    // Nothing is being built yet: what's made goes to trade goods.
    const int ratio = static_cast<int>(p_.integer(
        "trade_goods_ratio", {}, player.picks[RacePicks::FantasticTraders] != 0 ? "fantastic_traders" : "normal"));
    const int trade2 = std::max(0, out.production - out.upkeep) * 2 * ratio / 100;
    int upkeep2 = 0;
    for (const auto& b : colony.buildings) {
        upkeep2 += 2 * static_cast<int>(p_.integer("building", {b}, "maintenance"));
    }
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
    if (p.orbit <= 0 || static_cast<std::size_t>(p.orbit) >= kOrbits) {
        return star + " " + prime;
    }
    return star + " " + std::string(kRoman[static_cast<std::size_t>(p.orbit)]);
}

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
    for (Colony& c : game.colonies) {
        const Player& player = game.players.at(static_cast<std::size_t>(c.player));
        const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
        c.growth += economy.growth(player, planet, c.population());
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
        // Until the screen that offers the applications: a creative race
        // gets them all, others the first.
        std::span<const int> techs = field.techs;
        if (player.picks[RacePicks::Creative] == 0) {
            techs = techs.first(std::min<std::size_t>(techs.size(), 1));
        }
        tree.learn(player.knowledge, field.id, techs);
        player.researched = keep ? std::max(0, player.researched - 2 * field.cost) : 0;
        player.research = 0;
    }
}

Game startGame(const Parameters& parameters, Galaxy galaxy, const std::vector<RacePicks>& picks,
               const std::vector<std::size_t>& banners) {
    if (picks.size() != galaxy.homes.size() || banners.size() != galaxy.homes.size()) {
        throw std::invalid_argument("startGame: one race and banner per home system");
    }
    const Economy economy(parameters);
    const TechTree tree(parameters);
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
