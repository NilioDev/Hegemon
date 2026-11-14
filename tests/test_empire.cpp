// SPDX-License-Identifier: GPL-3.0-or-later
#include "tech_list.hpp"
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/random.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The settings an empire needs, in the fan patch's layout, with the
// classic values.
const std::string kList = R"({"patch_version": "test", "parameters": [
 {"name": "food_farmer_base", "type": "table", "rows": [], "default": [0, 0, 0, 2, 2, 4, 4, 2, 4, 6],
  "columns": [{"name": "toxic"}, {"name": "radiated"}, {"name": "barren"}, {"name": "desert"}, {"name": "tundra"},
              {"name": "ocean"}, {"name": "swamp"}, {"name": "arid"}, {"name": "terran"}, {"name": "gaia"}]},
 {"name": "production_worker_base", "type": "table", "rows": [], "default": [1, 2, 3, 5, 8],
  "columns": [{"name": "ultra_poor"}, {"name": "poor"}, {"name": "abundant"}, {"name": "rich"}, {"name": "ultra_rich"}]},
 {"name": "research_scientist_bonus", "type": "table", "rows": [], "default": [3, 2],
  "columns": [{"name": "base_scientist"}, {"name": "artifacts_planet"}]},
 {"name": "race_pick", "type": "table", "rows": [], "default": [-50, 50, 100, -1, 2, 4, -1, 1, 2, -1, 1, 2, -1, 1, 2],
  "columns": [{"name": "growth1_value"}, {"name": "growth2_value"}, {"name": "growth3_value"},
              {"name": "farming1_value"}, {"name": "farming2_value"}, {"name": "farming3_value"},
              {"name": "industry1_value"}, {"name": "industry2_value"}, {"name": "industry3_value"},
              {"name": "science1_value"}, {"name": "science2_value"}, {"name": "science3_value"},
              {"name": "money1_value"}, {"name": "money2_value"}, {"name": "money3_value"}]},
 {"name": "govt_bonus", "type": "table", "rows": [], "default": [-10, 10, 10, 10, 10],
  "columns": [{"name": "feudal_science"}, {"name": "democracy_science"}, {"name": "democracy_money"},
              {"name": "unification_food"}, {"name": "unification_production"}]},
 {"name": "planet_no_pollution_prod", "type": "table", "rows": [], "default": [2, 4, 6, 8, 10],
  "columns": [{"name": "tiny"}, {"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "trade_goods_ratio", "type": "table", "rows": [], "default": [50, 100],
  "columns": [{"name": "normal"}, {"name": "fantastic_traders"}]},
 {"name": "building", "type": "table", "rows": [["capitol", "marine_barracks", "star_base"]],
  "default": [[0], [1], [2]], "columns": [{"name": "maintenance"}]},
 {"name": "command_points", "type": "table", "rows": [], "default": [5, 1, 2, 3, 2],
  "columns": [{"name": "starting"}, {"name": "star_base"}, {"name": "battlestation"}, {"name": "star_fortress"},
              {"name": "warlord_planet"}]},
 {"name": "ship_command_points_usage", "type": "table", "rows": [], "default": [1, 2],
  "columns": [{"name": "frigate"}, {"name": "destroyer"}]},
 {"name": "starting_condition", "type": "table", "rows": [], "default": [8, 3, 50],
  "columns": [{"name": "starting_population"}, {"name": "starting_marines"}, {"name": "starting_money"}]},
 {"name": "no_starting_ships", "type": "integer", "min": 0, "max": 1, "default": 0},
 {"name": "starting_outpost_ship", "type": "integer", "min": 0, "max": 1, "default": 0},
 {"name": "pop_max", "type": "table", "rows": [], "default": [5, 10, 15, 20, 25],
  "columns": [{"name": "tiny"}, {"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "pop_climate", "type": "table", "rows": [], "default": [25, 25, 25, 25, 25, 25, 40, 60, 80, 100],
  "columns": [{"name": "toxic"}, {"name": "radiated"}, {"name": "barren"}, {"name": "desert"}, {"name": "tundra"},
              {"name": "ocean"}, {"name": "swamp"}, {"name": "arid"}, {"name": "terran"}, {"name": "gaia"}]},
 {"name": "pop_bonus_tolerant", "type": "integer", "min": 0, "max": 255, "default": 25},
 {"name": "pop_bonus_subterranean", "type": "table", "rows": [], "default": [2, 4, 6, 8, 10],
  "columns": [{"name": "tiny"}, {"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "growth_formula_factor", "type": "integer", "min": 0, "max": 65535, "default": 2000},
 {"name": "growth_rate_bonus", "type": "table", "rows": [], "default": [0, 25, 50, 100, -200],
  "columns": [{"name": "general_modifier"}, {"name": "microbiotics_tech"}, {"name": "antidote_tech"},
              {"name": "boom_event"}, {"name": "plague_event"}]},
)" + test::techSettings() + "]}";

Planet home(Climate climate = Climate::Terran, PlanetSize size = PlanetSize::Medium,
            Minerals minerals = Minerals::Abundant) {
    Planet p;
    p.climate = climate;
    p.size = size;
    p.minerals = minerals;
    return p;
}

Player race(std::initializer_list<std::pair<RacePicks::Pick, int>> picks, int government = 2) {
    Player p;
    p.picks.values[RacePicks::Government] = static_cast<std::int8_t>(government);
    for (const auto& [pick, value] : picks) {
        p.picks.values[pick] = static_cast<std::int8_t>(value);
    }
    return p;
}

// A one-player game on the planet.
Game game(const Parameters& p, const Player& player, const Planet& planet) {
    Galaxy g;
    Star s;
    g.stars.push_back(s);
    g.planets.push_back(planet);
    g.homes.push_back({player.race, 0, 0});
    return startGame(p, g, {player.picks}, {0});
}

} // namespace

HGM_TEST("Empire: a new home colony feeds its people, then splits the rest") {
    const Parameters p = Parameters::defaults(kList);
    const Economy e(p);
    using J = std::array<int, kJobs>;
    // Farmer, worker, scientist; measured on the stock races.
    HGM_CHECK((e.startingJobs(race({}), home(), 8) == J{4, 1, 3}));
    HGM_CHECK((e.startingJobs(race({}, 4), home(), 8) == J{4, 2, 2}));                      // democracy
    HGM_CHECK((e.startingJobs(race({{RacePicks::Science, 3}}), home(), 8) == J{4, 2, 2})); // +2 research
    HGM_CHECK((e.startingJobs(race({{RacePicks::Farming, 2}}, 0), home(Climate::Terran, PlanetSize::Large), 8) == J{3, 1, 4})); // feudal
    HGM_CHECK((e.startingJobs(race({{RacePicks::Farming, 2}}, 6), home(), 8) == J{2, 2, 4}));  // unification
    HGM_CHECK((e.startingJobs(race({{RacePicks::Cybernetic, 1}}), home(), 8) == J{2, 2, 4}));
    HGM_CHECK((e.startingJobs(race({{RacePicks::Lithovore, 1}}), home(), 8) == J{0, 3, 5}));
    HGM_CHECK((e.startingJobs(race({{RacePicks::Aquatic, 1}}), home(Climate::Ocean), 8) == J{3, 2, 3}));
    HGM_CHECK((e.startingJobs(race({}), home(), 1) == J{1, 0, 0}));
}

HGM_TEST("Empire: a new game's home colonies, ships and the side panel's numbers") {
    const Parameters p = Parameters::defaults(kList);
    const Economy e(p);
    {
        const Game g = game(p, race({}), home());
        HGM_CHECK(g.colonies.size() == 1 && g.colonies[0].population() == 8);
        HGM_CHECK(g.colonies[0].has("capitol") && g.colonies[0].has("star_base") && g.colonies[0].has("marine_barracks"));
        HGM_CHECK(g.ships.size() == 3 && g.ships[2].kind == ShipKind::ColonyShip);
        const EmpireSummary s = e.summary(g, 0);
        // 8 BC taxes and 1.5 BC of trade goods, less 3 BC upkeep: +6.5
        // shows as +7.
        HGM_CHECK(s.money == 50 && s.income == 7 && s.food == 0);
        HGM_CHECK(s.commandPoints == 6 && s.commandUsed == 3);
    }
    {
        // Unification: no capitol; +50 % production before pollution, +50 %
        // food.
        const Game g = game(p, race({{RacePicks::Farming, 2}, {RacePicks::Industry, 2}}, 6), home(Climate::Terran, PlanetSize::Large));
        HGM_CHECK(!g.colonies[0].has("capitol"));
        const ColonyOutput o = e.output(g, g.colonies[0]);
        HGM_CHECK(o.production == 10); // 2 × 4 × 1.5 = 12, less half of 4 over 8
        const EmpireSummary s = e.summary(g, 0);
        HGM_CHECK(s.income == 10 && s.food == 1);
    }
    {
        // Cybernetic people need half a production each.
        const Game g = game(p, race({{RacePicks::Industry, 3}, {RacePicks::Cybernetic, 1}}), home());
        const ColonyOutput o = e.output(g, g.colonies[0]);
        HGM_CHECK(o.production == 8 && o.upkeep == 4);
        HGM_CHECK(e.summary(g, 0).income == 7);
    }
    {
        // Democracy: +50 % taxes. Fantastic traders: trade goods 1:1.
        HGM_CHECK(e.summary(game(p, race({}, 4), home()), 0).income == 12);
        HGM_CHECK(e.summary(game(p, race({{RacePicks::Money, 3}, {RacePicks::FantasticTraders, 1}}), home()), 0).income == 16);
        // A warlord gets two command points for its home world.
        const EmpireSummary s = e.summary(game(p, race({{RacePicks::Warlord, 1}}, 2), home(Climate::Terran, PlanetSize::Medium, Minerals::Rich)), 0);
        HGM_CHECK(s.commandPoints == 8 && s.income == 8);
    }
}

HGM_TEST("Empire: the stock races start as measured on the original") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Races races(data);
    const Economy e(data.parameters());
    struct Expected {
        std::array<int, kJobs> jobs;
        int income, food, commandPoints;
    };
    // Alkari to Trilarians, the human's home colony at the start.
    const Expected expected[Races::kStockRaces] = {
        {{4, 2, 2}, 8, 0, 6},  {{4, 1, 3}, 7, 0, 6},  {{4, 1, 3}, 7, 0, 6}, {{4, 1, 3}, 7, 0, 6},
        {{4, 1, 3}, 16, 0, 6}, {{4, 2, 2}, 12, 0, 6}, {{2, 2, 4}, 10, 1, 6}, {{2, 2, 4}, 7, 0, 6},
        {{4, 1, 3}, 8, 0, 8},  {{4, 2, 2}, 8, 0, 6},  {{3, 1, 4}, 7, 1, 6}, {{0, 3, 5}, 10, 0, 6},
        {{3, 2, 3}, 8, 1, 6},
    };
    for (std::size_t r = 0; r < Races::kStockRaces; ++r) {
        Player player;
        player.race = static_cast<int>(r);
        player.picks = races.stock(r);
        const auto& k = player.picks;
        Planet planet = home(k[RacePicks::Aquatic] != 0 ? Climate::Ocean : Climate::Terran,
                             k[RacePicks::LargeHomeWorld] != 0 ? PlanetSize::Large : PlanetSize::Medium,
                             k[RacePicks::HomeWorldRichness] == 1 ? Minerals::Rich : Minerals::Abundant);
        if (k[RacePicks::ArtifactsWorld] != 0) {
            planet.special = Special::AncientArtifacts;
        }
        const Game g = game(data.parameters(), player, planet);
        const EmpireSummary s = e.summary(g, 0);
        HGM_CHECK(g.colonies[0].jobs == expected[r].jobs);
        HGM_CHECK(s.money == 50 && s.income == expected[r].income && s.food == expected[r].food);
        HGM_CHECK(s.commandPoints == expected[r].commandPoints && s.commandUsed == 3);
        if (g.colonies[0].jobs != expected[r].jobs || s.income != expected[r].income || s.food != expected[r].food) {
            std::printf("  %s: %d/%d/%d income %d food %d\n", std::string(Races::kNames[r]).c_str(), g.colonies[0].jobs[0],
                        g.colonies[0].jobs[1], g.colonies[0].jobs[2], s.income, s.food);
        }
    }
}

HGM_TEST("Empire: a turn adds income and research, and a field is learnt by twice its cost") {
    const Parameters p = Parameters::defaults(kList);
    const TechTree tree(p);
    Game g;
    Player player;
    player.money = 50;
    player.knowledge = tree.averageStart();
    player.research = 13;
    g.players.push_back(player);
    Random random(1);
    endTurn(g, p, tree, random);
    HGM_CHECK(g.stardate == 35001 && g.players[0].money == 50 && g.players[0].researched == 0);
    // Without colonies there is no income or research; put in all but the
    // last point.
    g.players[0].researched = 159;
    endTurn(g, p, tree, random);
    // 159 of 80 may or may not have broken through; 160 always does.
    if (g.players[0].research != 0) {
        g.players[0].researched = 160;
        endTurn(g, p, tree, random);
    }
    HGM_CHECK(g.players[0].research == 0 && g.players[0].researched == 0);
    HGM_CHECK(g.players[0].knowledge.knowsField(13) && g.players[0].knowledge.knowsTech(3));
    HGM_CHECK(!g.players[0].knowledge.knowsTech(4)); // one application, until there is a choice

    // A new game's empire: +7 BC and three scientists' 9 RP a turn.
    Game h = game(p, race({}), home());
    h.players[0].research = 13;
    endTurn(h, p, tree, random);
    HGM_CHECK(h.players[0].money == 57 && h.players[0].researched == 9);
    const EmpireSummary s = Economy(p).summary(h, 0);
    HGM_CHECK(s.research == 9 && s.researchField == 13 && s.researchTurns == 13);
}

HGM_TEST("Empire: how many people a planet holds and how fast they grow") {
    const Parameters p = Parameters::defaults(kList);
    const Economy e(p);
    // Measured on the stock races' home colonies of eight: the game's own
    // growth a turn.
    HGM_CHECK(e.maxPopulation(race({}), home()) == 12 && e.growth(race({}), home(), 8) == 73);
    HGM_CHECK(e.maxPopulation(race({{RacePicks::Subterranean, 1}}), home(Climate::Terran, PlanetSize::Large)) == 24);
    HGM_CHECK(e.growth(race({{RacePicks::Subterranean, 1}, {RacePicks::Growth, 3}}), home(Climate::Terran, PlanetSize::Large), 8) ==
              206); // Sakkra
    HGM_CHECK(e.maxPopulation(race({{RacePicks::Tolerant, 1}}), home()) == 15);
    HGM_CHECK(e.growth(race({{RacePicks::Tolerant, 1}, {RacePicks::Growth, 1}}), home(), 8) == 43); // Silicoids
    HGM_CHECK(e.growth(race({{RacePicks::Aquatic, 1}}), home(Climate::Ocean), 8) == 86);           // Trilarians
    HGM_CHECK(e.growth(race({}), home(), 12) == 0 && e.growth(race({}), home(), 0) == 0);

    // A turn grows the colony; at a thousand there is one more person.
    const TechTree tree(p);
    Random random(1);
    Game g = game(p, race({}), home());
    endTurn(g, p, tree, random);
    HGM_CHECK(g.colonies[0].population() == 8 && g.colonies[0].growth == 73);
    g.colonies[0].growth = 990;
    endTurn(g, p, tree, random);
    HGM_CHECK(g.colonies[0].population() == 9 && g.colonies[0].growth == 63);
}

HGM_TEST("Empire: planets are named after their star and orbit") {
    Galaxy g;
    Star s;
    s.name = "Altair";
    g.stars.push_back(s);
    for (int orbit = 0; orbit < 3; ++orbit) {
        Planet planet;
        planet.orbit = orbit;
        g.planets.push_back(planet);
    }
    HGM_CHECK(planetName(g, 0, "Prime") == "Altair Prime");
    HGM_CHECK(planetName(g, 2, "Prime") == "Altair III");
}
