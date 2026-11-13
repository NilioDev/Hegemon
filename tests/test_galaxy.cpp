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
    const GalaxyGenerator gen(data.parameters(), StarNames::load(data));
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
