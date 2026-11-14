// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/galaxy_generator.hpp"
#include "hegemon/rules/random.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// Map generation tables with made-up numbers, in the fan patch's layout:
// red stars only, every satellite a planet, sizes and climates fixed.
constexpr const char* kList = R"({"patch_version": "test", "parameters": [
 {"name": "map_generation", "type": "table", "rows": [["small", "medium", "large", "huge"]],
  "default": [[506, 400, 10, 10, 0, 0], [759, 600, 15, 15, 1, 1], [1012, 800, 20, 20, 2, 2], [1518, 1200, 30, 30, 3, 3]],
  "columns": [{"name": "width"}, {"name": "height"}, {"name": "star_spacing"}, {"name": "zoom_factor"},
              {"name": "max_zoom_count"}, {"name": "zoom_count"}]},
 {"name": "stars_in_galaxy", "type": "table", "rows": [], "default": [12, 24, 30, 40],
  "columns": [{"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "star_class_chance", "type": "table",
  "rows": [["blue", "white", "yellow", "orange", "red", "brown", "black_hole"]],
  "default": [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [1, 1, 1], [0, 0, 0], [0, 0, 0]],
  "columns": [{"name": "young"}, {"name": "average"}, {"name": "old"}]},
 {"name": "class_to_num_satellites", "type": "table",
  "rows": [["random0", "random1", "random2", "random3", "random4", "random5", "random6", "random7", "random8", "random9"]],
  "default": 2, "columns": [{"name": "blue"}, {"name": "white"}, {"name": "yellow"}, {"name": "orange"}, {"name": "red"}, {"name": "brown"}]},
 {"name": "satellite_orbit_chance", "type": "table", "rows": [["orbit1", "orbit2", "orbit3", "orbit4", "orbit5"]],
  "default": 1, "columns": [{"name": "young"}, {"name": "average"}, {"name": "old"}]},
 {"name": "orbit_to_satellite_type", "type": "table",
  "rows": [["random0", "random1", "random2", "random3", "random4", "random5", "random6", "random7", "random8", "random9"]],
  "default": 3, "columns": [{"name": "orbit1"}, {"name": "orbit2"}, {"name": "orbit3"}, {"name": "orbit4"}, {"name": "orbit5"}]},
 {"name": "planet_size_chance", "type": "table", "rows": [], "default": [0, 0, 10, 10, 10],
  "columns": [{"name": "tiny"}, {"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "class_to_mineral", "type": "table",
  "rows": [["random0", "random1", "random2", "random3", "random4", "random5", "random6", "random7", "random8", "random9"]],
  "default": 1, "columns": [{"name": "blue"}, {"name": "white"}, {"name": "yellow"}, {"name": "orange"}, {"name": "red"}, {"name": "brown"}]},
 {"name": "planet_size_to_gravity", "type": "table", "rows": [["ultra_poor", "poor", "abundant", "rich", "ultra_rich"]],
  "default": 2, "columns": [{"name": "tiny"}, {"name": "small"}, {"name": "medium"}, {"name": "large"}, {"name": "huge"}]},
 {"name": "orbit_to_climate_group", "type": "table", "rows": [["blue", "white", "yellow", "orange", "red", "brown"]],
  "default": 3, "columns": [{"name": "orbit1"}, {"name": "orbit2"}, {"name": "orbit3"}, {"name": "orbit4"}, {"name": "orbit5"}]},
 {"name": "climate_chance_young_avg_gal", "type": "table",
  "rows": [["toxic", "radiated", "barren", "desert", "tundra", "ocean", "swamp", "arid", "terran", "gaia"]],
  "default": [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 1], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 1], [0, 0, 0, 0]],
  "columns": [{"name": "bombarded"}, {"name": "hostile"}, {"name": "life_supporting"}, {"name": "lifeless"}]},
 {"name": "mapgen", "type": "table", "rows": [], "default": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  "columns": [{"name": "-ttoxic"}, {"name": "-tupoor"}, {"name": "-tlowg"}, {"name": "-theavyg"}, {"name": "-ttiny"}, {"name": "-tsmall"}, {"name": "-tflathw"}, {"name": "-tfixedhw"}, {"name": "-tgaia"}, {"name": "-trichhw"}, {"name": "-thugehw"}, {"name": "-tgoodhw"}, {"name": "-ssplint"}, {"name": "-snatives"}, {"name": "-sarti"}, {"name": "-shero"}, {"name": "-smarooned"}, {"name": "-mgrav"}, {"name": "-mterraform"}, {"name": "-mmonst"}, {"name": "-bshowring"}, {"name": "/monsters"}, {"name": "/planets"}, {"name": "/minstart"}, {"name": "/hugestart"}, {"name": "/richstart"}, {"name": "/goodstart"}, {"name": "/nobh"}, {"name": "/nowh"}, {"name": "/nonebula"}, {"name": "/nosplint"}, {"name": "/noorion"}]},
 {"name": "food_farmer_base", "type": "table", "rows": [], "default": [0, 0, 0, 2, 2, 4, 4, 2, 4, 6],
  "columns": [{"name": "toxic"}, {"name": "radiated"}, {"name": "barren"}, {"name": "desert"}, {"name": "tundra"},
              {"name": "ocean"}, {"name": "swamp"}, {"name": "arid"}, {"name": "terran"}, {"name": "gaia"}]},
 {"name": "star_special_chance", "type": "integer", "min": 0, "max": 100, "default": 100},
 {"name": "planet_special_chance", "type": "table", "rows": [], "default": [0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0],
  "columns": [{"name": "no_special"}, {"name": "wormhole"}, {"name": "space_debris"}, {"name": "pirate_cache"},
              {"name": "gold_deposits"}, {"name": "gem_deposits"}, {"name": "natives"}, {"name": "splinter_colony"},
              {"name": "hero"}, {"name": "monster"}, {"name": "ancient_artifacts"}, {"name": "unused?"}]},
 {"name": "climate_chance_old_gal", "type": "table",
  "rows": [["toxic", "radiated", "barren", "desert", "tundra", "ocean", "swamp", "arid", "terran", "gaia"]],
  "default": 0, "columns": [{"name": "bombarded"}, {"name": "hostile"}, {"name": "life_supporting"}, {"name": "lifeless"}]}
]})";

StarNames testNames() {
    StarNames n;
    for (int i = 0; i < 13; ++i) {
        n.home.push_back("Home" + std::to_string(i));
    }
    for (int i = 0; i < 100; ++i) {
        n.other.push_back("Star" + std::to_string(i));
    }
    return n;
}

GalaxySettings settings(int players) {
    GalaxySettings s;
    for (int i = 0; i < players; ++i) {
        GalaxyPlayer p;
        p.race = i;
        s.players.push_back(p);
    }
    return s;
}

} // namespace

HGM_TEST("Random: the same seed gives the same numbers, in range") {
    Random a(42), b(42), c(43);
    bool differs = false;
    for (int i = 0; i < 100; ++i) {
        const auto x = a.next();
        HGM_CHECK(x == b.next());
        differs = differs || x != c.next();
    }
    HGM_CHECK(differs);
    for (int i = 0; i < 1000; ++i) {
        const int v = a.between(-3, 3);
        HGM_CHECK(v >= -3 && v <= 3);
        HGM_CHECK(a.below(7) < 7);
    }
    const std::int64_t none[] = {0, 0, -5};
    HGM_CHECK(!a.pick(none).has_value());
    const std::int64_t one[] = {0, 3, 0};
    HGM_CHECK(a.pick(one) == 1u);
}

HGM_TEST("Galaxy: stars apart, homes apart, planets from the tables") {
    const Parameters p = Parameters::defaults(kList);
    const GalaxyGenerator gen(p, testNames());
    Random r(7);
    auto s = settings(4);
    s.players[1].picks.values[RacePicks::LargeHomeWorld] = 1;
    s.players[2].picks.values[RacePicks::HighGravity] = 1;
    s.players[3].picks.values[RacePicks::Aquatic] = 1;
    s.players[3].picks.values[RacePicks::HomeWorldRichness] = 1;
    for (int round = 0; round < 20; ++round) {
        const Galaxy g = gen.generate(s, r);
        HGM_CHECK(g.width == 759 && g.height == 600);
        HGM_CHECK(g.stars.size() == 24);
        HGM_CHECK(g.stars[0].special == Special::Orion);
        for (const Planet& planet : g.planets) {
            HGM_CHECK(planet.image >= 0 && planet.image < kPlanetImages);
        }
        std::set<std::string> names;
        for (std::size_t i = 0; i < g.stars.size(); ++i) {
            const Star& a = g.stars[i];
            names.insert(a.name);
            HGM_CHECK(a.x >= 0 && a.x < g.width && a.y >= 0 && a.y < g.height);
            for (std::size_t j = i + 1; j < g.stars.size(); ++j) {
                const Star& b = g.stars[j];
                HGM_CHECK(std::abs(a.x - b.x) >= 60 || std::abs(a.y - b.y) >= 74);
            }
            for (std::size_t o = 0; o < kOrbits; ++o) {
                if (a.orbits[o] != kNoPlanet) {
                    const Planet& q = g.planets.at(static_cast<std::size_t>(a.orbits[o]));
                    HGM_CHECK(q.star == static_cast<int>(i) && q.orbit == static_cast<int>(o));
                }
            }
        }
        HGM_CHECK(names.size() == g.stars.size());
        for (std::size_t i = 1; i < g.stars.size(); ++i) {
            const bool home = std::any_of(g.homes.begin(), g.homes.end(),
                                          [&](const HomeSystem& h) { return h.star == static_cast<int>(i); });
            if (!home) {
                HGM_CHECK(g.stars[i].satellites() == 2);
            }
        }
        HGM_CHECK(g.homes.size() == 4);
        for (std::size_t i = 0; i < g.homes.size(); ++i) {
            const HomeSystem& h = g.homes[i];
            const Star& star = g.stars.at(static_cast<std::size_t>(h.star));
            HGM_CHECK(star.name == "Home" + std::to_string(i));
            HGM_CHECK(star.colour != StarColour::BlackHole);
            HGM_CHECK(star.satellites() >= 3);
            for (std::size_t j = i + 1; j < g.homes.size(); ++j) {
                const Star& other = g.stars.at(static_cast<std::size_t>(g.homes[j].star));
                HGM_CHECK(std::hypot(star.x - other.x, star.y - other.y) >= 290.0);
            }
            const Planet& world = g.planets.at(static_cast<std::size_t>(h.planet));
            HGM_CHECK(world.star == h.star);
            HGM_CHECK(world.size == (i == 1 ? PlanetSize::Large : PlanetSize::Medium));
            HGM_CHECK(world.gravity == (i == 2 ? Gravity::Heavy : Gravity::Normal));
            HGM_CHECK(world.climate == (i == 3 ? Climate::Ocean : Climate::Terran));
            HGM_CHECK(world.minerals == (i == 3 ? Minerals::Rich : Minerals::Abundant));
            // The others of the system aren't Terran.
            for (const int o : star.orbits) {
                if (o != kNoPlanet && o != h.planet) {
                    HGM_CHECK(g.planets.at(static_cast<std::size_t>(o)).climate != Climate::Terran);
                }
            }
        }
        for (const Planet& q : g.planets) {
            if (q.special == Special::Orion) {
                HGM_CHECK(q.size == PlanetSize::Huge && q.climate == Climate::Gaia);
                continue;
            }
            if (q.star == 0) {
                HGM_CHECK(q.type == SatelliteType::AsteroidBelt);
                continue;
            }
            HGM_CHECK(q.type == SatelliteType::Planet);
            HGM_CHECK(g.stars.at(static_cast<std::size_t>(q.star)).colour != StarColour::BlackHole);
            const bool home = std::any_of(g.homes.begin(), g.homes.end(), [&](const HomeSystem& h) {
                return &g.planets.at(static_cast<std::size_t>(h.planet)) == &q;
            });
            if (!home) {
                HGM_CHECK(q.size == PlanetSize::Medium);
                HGM_CHECK(q.minerals == Minerals::Poor);
                HGM_CHECK(q.gravity == Gravity::Heavy);
                HGM_CHECK(q.climate == Climate::Tundra || q.climate == Climate::Terran);
            }
        }
    }
}

HGM_TEST("Galaxy: the same seed makes the same galaxy") {
    const Parameters p = Parameters::defaults(kList);
    const GalaxyGenerator gen(p, testNames());
    Random a(99), b(99);
    const Galaxy x = gen.generate(settings(3), a);
    const Galaxy y = gen.generate(settings(3), b);
    HGM_CHECK(x.stars.size() == y.stars.size());
    for (std::size_t i = 0; i < x.stars.size(); ++i) {
        HGM_CHECK(x.stars[i].x == y.stars[i].x && x.stars[i].y == y.stars[i].y);
        HGM_CHECK(x.stars[i].name == y.stars[i].name);
    }
    HGM_CHECK(x.planets.size() == y.planets.size());
}

HGM_TEST("Galaxy: a cluster has a large map and a huge galaxy's stars") {
    const Parameters p = Parameters::defaults(kList);
    const GalaxyGenerator gen(p, testNames());
    Random r(5);
    auto s = settings(5);
    s.size = NewGameSettings::GalaxySize::Cluster;
    const Galaxy g = gen.generate(s, r);
    HGM_CHECK(g.cluster);
    HGM_CHECK(g.width == 1012 && g.height == 800);
    HGM_CHECK(g.stars.size() == 40);
}

HGM_TEST("Galaxy: a planet that grows food near each home of a race that eats") {
    // Only gaia grows food, and no planet is gaia: each home but the
    // Lithovores' gets one within six parsecs.
    std::string list = kList;
    const std::string food = "[0, 0, 0, 2, 2, 4, 4, 2, 4, 6]";
    list.replace(list.find(food), food.size(), "[0, 0, 0, 0, 0, 0, 0, 0, 0, 6]");
    const Parameters p = Parameters::defaults(list);
    const GalaxyGenerator gen(p, testNames());
    Random r(5);
    auto s = settings(4);
    s.players[3].picks.values[RacePicks::Lithovore] = 1;
    for (int round = 0; round < 20; ++round) {
        const Galaxy g = gen.generate(s, r);
        for (std::size_t i = 0; i < 3; ++i) {
            const Star& home = g.stars.at(static_cast<std::size_t>(g.homes[i].star));
            const bool fed = std::any_of(g.planets.begin(), g.planets.end(), [&](const Planet& q) {
                const Star& t = g.stars.at(static_cast<std::size_t>(q.star));
                const bool otherHome = std::any_of(g.homes.begin(), g.homes.end(),
                                                   [&](const HomeSystem& h) { return h.star == q.star; });
                const bool guarded = std::any_of(g.monsters.begin(), g.monsters.end(),
                                                 [&](const Monster& m) { return m.star == q.star; });
                return q.type == SatelliteType::Planet && q.climate == Climate::Gaia && q.special != Special::Orion &&
                       !otherHome && !guarded && std::hypot(t.x - home.x, t.y - home.y) <= 180.0;
            });
            HGM_CHECK(fed);
        }
    }
}

HGM_TEST("Galaxy: special finds, wormholes and monsters") {
    std::string list = kList;
    const std::string never = R"("max": 100, "default": 100})";
    list.replace(list.find(never), never.size(), R"("max": 100, "default": 0})");
    const Parameters p = Parameters::defaults(list);
    const GalaxyGenerator gen(p, testNames());
    Random r(11);
    auto s = settings(4);
    s.players[0].picks.values[RacePicks::ArtifactsWorld] = 1;
    int wormholes = 0, finds = 0;
    for (int round = 0; round < 20; ++round) {
        const Galaxy g = gen.generate(s, r);
        const auto home = [&](int i) {
            return std::any_of(g.homes.begin(), g.homes.end(), [&](const HomeSystem& h) { return h.star == i; });
        };
        HGM_CHECK(g.planets.at(static_cast<std::size_t>(g.homes[0].planet)).special == Special::AncientArtifacts);
        for (int i = 1; i < static_cast<int>(g.stars.size()); ++i) {
            const Star& star = g.stars[static_cast<std::size_t>(i)];
            if (home(i)) {
                continue;
            }
            // Every other star with satellites has a find or a wormhole.
            HGM_CHECK(star.satellites() == 0 || star.special != Special::None || star.wormhole != -1);
            if (star.wormhole != -1) {
                ++wormholes;
                const Star& end = g.stars.at(static_cast<std::size_t>(star.wormhole));
                HGM_CHECK(end.wormhole == i);
                HGM_CHECK(std::hypot(star.x - end.x, star.y - end.y) >= 120.0);
                HGM_CHECK(star.special == Special::None);
            }
            if (star.special != Special::None) {
                ++finds;
            }
        }
        for (const Planet& q : g.planets) {
            if (q.special == Special::Natives || q.special == Special::SplinterColony) {
                HGM_CHECK(q.size >= PlanetSize::Medium);
                HGM_CHECK(q.climate >= Climate::Desert);
            }
            if (q.special != Special::None) {
                HGM_CHECK(g.stars.at(static_cast<std::size_t>(q.star)).special == q.special);
            }
        }
        HGM_CHECK(!g.monsters.empty() && g.monsters[0].kind == MonsterKind::Guardian && g.monsters[0].star == 0);
        HGM_CHECK(g.monsters.size() <= 4);
        for (std::size_t m = 1; m < g.monsters.size(); ++m) {
            HGM_CHECK(g.monsters[m].kind != MonsterKind::Guardian);
            HGM_CHECK(g.stars.at(static_cast<std::size_t>(g.monsters[m].star)).special != Special::None);
        }
    }
    HGM_CHECK(wormholes > 0);
    HGM_CHECK(finds > 0);
}

HGM_TEST("Galaxy: nebulae, stars in them, no black holes there") {
    std::string list = kList;
    // Black holes only, so every star outside a nebula is one.
    const std::string red = R"([[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [1, 1, 1], [0, 0, 0], [0, 0, 0]])";
    list.replace(list.find(red), red.size(), R"([[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [1, 1, 1], [0, 0, 0], [1000, 1000, 1000]])");
    const Parameters p = Parameters::defaults(list);
    // Twelve square nebulae, bright inside a dark rim.
    std::vector<Canvas> pictures;
    for (int i = 0; i < 12; ++i) {
        Canvas c(150, 140, 0);
        for (int y = 10; y < 130; ++y) {
            for (int x = 10; x < 140; ++x) {
                c.set(x, y, 20);
            }
        }
        pictures.push_back(c);
    }
    const GalaxyGenerator gen(p, testNames(), pictures);
    Random r(3);
    auto s = settings(3);
    s.size = NewGameSettings::GalaxySize::Huge;
    for (auto& player : s.players) { // black holes grow no food
        player.picks.values[RacePicks::Lithovore] = 1;
    }
    int inside = 0;
    for (int round = 0; round < 10; ++round) {
        const Galaxy g = gen.generate(s, r);
        HGM_CHECK(g.nebulae.size() >= 2 && g.nebulae.size() <= 4);
        for (const auto& b : g.nebulae) {
            HGM_CHECK(b.picture >= 0 && b.picture < 12);
            HGM_CHECK(b.x >= 0 && b.y >= 0 && b.x + 150 <= g.width && b.y + 140 <= g.height);
        }
        for (const Star& star : g.stars) {
            const bool in = std::any_of(g.nebulae.begin(), g.nebulae.end(), [&](const Nebula& b) {
                return star.x >= b.x + 10 && star.x < b.x + 140 && star.y >= b.y + 10 && star.y < b.y + 130;
            });
            HGM_CHECK(star.inNebula == in);
            if (in) {
                ++inside;
                HGM_CHECK(star.colour != StarColour::BlackHole);
            }
        }
    }
    HGM_CHECK(inside > 0);
}

HGM_TEST("Galaxy: the fan patch's map switches") {
    // In the order of the mapgen table's columns.
    std::array<int, 32> v{};
    v[21] = 2;  // /monsters: two guarded systems
    v[22] = 5;  // /planets: five in each home system
    v[0] = 1;   // -ttoxic
    v[11] = 1;  // -tgoodhw
    v[13] = 1;  // -snatives
    v[27] = 1;  // /nobh
    v[28] = 1;  // /nowh
    v[31] = 1;  // /noorion
    std::string switches = "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        switches += (i ? ", " : "") + std::to_string(v[i]);
    }
    switches += "]";
    std::string list = kList;
    const std::string plain = "[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0]";
    list.replace(list.find(plain), plain.size(), switches);
    const std::string never = R"("max": 100, "default": 100})";
    list.replace(list.find(never), never.size(), R"("max": 100, "default": 0})");
    const Parameters p = Parameters::defaults(list);
    const GalaxyGenerator gen(p, testNames());
    Random r(17);
    for (int round = 0; round < 10; ++round) {
        const Galaxy g = gen.generate(settings(3), r);
        HGM_CHECK(g.stars.size() == 24);
        for (const Star& star : g.stars) {
            HGM_CHECK(star.special != Special::Orion && star.special != Special::Natives);
            HGM_CHECK(star.wormhole == -1 && star.colour != StarColour::BlackHole);
        }
        HGM_CHECK(g.monsters.size() == 2);
        for (const Monster& m : g.monsters) {
            HGM_CHECK(m.kind != MonsterKind::Guardian);
        }
        for (const Planet& q : g.planets) {
            HGM_CHECK(q.climate != Climate::Toxic);
        }
        for (const HomeSystem& h : g.homes) {
            const Star& star = g.stars.at(static_cast<std::size_t>(h.star));
            HGM_CHECK(star.satellites() == 5);
            // The free planet nearest the star: rich, huge and terran.
            for (const int o : star.orbits) {
                if (o != h.planet) {
                    const Planet& q = g.planets.at(static_cast<std::size_t>(o));
                    HGM_CHECK(q.minerals == Minerals::Rich && q.size == PlanetSize::Huge && q.climate == Climate::Terran);
                    break;
                }
            }
        }
    }
}

HGM_TEST("Galaxy: too many players for the stars") {
    const Parameters p = Parameters::defaults(kList);
    const GalaxyGenerator gen(p, testNames());
    Random r(1);
    auto s = settings(12);
    s.size = NewGameSettings::GalaxySize::Small;
    bool threw = false;
    try {
        (void)gen.generate(s, r);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    HGM_CHECK(threw);
}

HGM_TEST("Galaxy: the fan patch's tables make galaxies like the original's") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!findFileNoCase(game, FanPatchConfig::kMainFile) || !findFileNoCase(game, "STARNAME.LBX")) {
        HGM_SKIP("fan patch 1.51 not installed in gamedata/");
    }
    GameData data(game);
    const Races races(data);
    const GalaxyGenerator gen(data.parameters(), StarNames::load(data), loadNebulae(data));
    GalaxySettings s;
    for (int i = 0; i < 5; ++i) {
        s.players.push_back({i, races.stock(static_cast<std::size_t>(i))});
    }
    Random r(2026);
    std::array<int, kStarColours> colours{};
    int stars = 0;
    for (int round = 0; round < 50; ++round) {
        const Galaxy g = gen.generate(s, r);
        HGM_CHECK(g.stars.size() == 36);
        HGM_CHECK(g.stars[0].name == "Orion");
        HGM_CHECK(g.stars.at(static_cast<std::size_t>(g.homes[0].star)).name == "Altair");
        for (const auto& star : g.stars) {
            ++colours[static_cast<std::size_t>(star.colour)];
            ++stars;
        }
    }
    // Red stars are the most common, about 37 in a hundred.
    const double red = 100.0 * colours[static_cast<std::size_t>(StarColour::Red)] / stars;
    HGM_CHECK(red > 30 && red < 45);
}
