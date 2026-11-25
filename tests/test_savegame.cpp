// SPDX-License-Identifier: GPL-3.0-or-later
#include "tech_list.hpp"
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/rules/savegame.hpp"
#include "hegemon/rules/tech.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace hegemon;

namespace {

const std::string kList = R"({"patch_version": "test", "parameters": [
 {"name": "map_generation", "type": "table", "rows": [["small", "medium", "large", "huge"]],
  "default": [[506, 400], [759, 600], [1012, 800], [1518, 1200]], "columns": [{"name": "width"}, {"name": "height"}]},
 {"name": "building", "type": "table", "rows": [["capitol", "colony_base", "marine_barracks"]],
  "default": [[8, 0], [11, 0], [24, 0]], "columns": [{"name": "id"}, {"name": "tech_id"}]},
)" + test::techSettings() + "]}";

void put8(std::vector<std::uint8_t>& b, std::size_t at, int v) { b.at(at) = static_cast<std::uint8_t>(v); }
void put16(std::vector<std::uint8_t>& b, std::size_t at, int v) {
    put8(b, at, v & 0xff);
    put8(b, at + 1, (v >> 8) & 0xff);
}
void putText(std::vector<std::uint8_t>& b, std::size_t at, const char* s) { std::memcpy(&b.at(at), s, std::strlen(s)); }

// A saved game as the measured ones lay it out: two stars, a colony at the
// first, a fleet on its way to the second.
std::vector<std::uint8_t> saved() {
    std::vector<std::uint8_t> b(kSavegameSize, 0);
    put16(b, 0, 0xe0);
    putText(b, 4, "Test Game");
    put16(b, 41, 35012);
    put16(b, 203753, 759);
    put16(b, 203755, 600);
    put8(b, 203779, 1); // one nebula
    put16(b, 203759, 324);
    put16(b, 203761, 189);
    put8(b, 203763, 7);
    for (std::size_t i = 0; i < 250; ++i) {
        put8(b, 603 + 361 * i + 2, 0xff); // no colony
    }
    // Stars: Sal (red, with the planet) and Derke.
    put16(b, 96977, 2);
    const std::size_t sal = 96979, derke = sal + 113;
    putText(b, sal, "Sal");
    put16(b, sal + 15, 100);
    put16(b, sal + 17, 200);
    put8(b, sal + 19, 2);
    put8(b, sal + 22, 4);
    put8(b, sal + 41, 0xff);
    put8(b, sal + 51, 1); // the first player has been there
    for (std::size_t o = 0; o < 5; ++o) {
        put16(b, sal + 74 + 2 * o, o == 1 ? 0 : 0xffff);
    }
    putText(b, derke, "Derke");
    put16(b, derke + 15, 300);
    put16(b, derke + 17, 50);
    put8(b, derke + 41, 0xff);
    for (std::size_t o = 0; o < 5; ++o) {
        put16(b, derke + 74 + 2 * o, 0xffff);
    }
    // The planet: a large poor ocean planet in Sal's second orbit.
    put16(b, 90855, 1);
    const std::size_t planet = 90857;
    put16(b, planet, 0);
    put8(b, planet + 2, 0);
    put8(b, planet + 3, 1);
    put8(b, planet + 4, 3);
    put8(b, planet + 5, 3);
    put8(b, planet + 6, 1);
    put8(b, planet + 8, 5);
    put8(b, planet + 9, 2);
    put8(b, planet + 10, 1);
    // The player: Alkari, banner 3, 63 BC, field 4 known, researching field
    // 13 for tech 3 with 25 points in it.
    const std::size_t player = 109071;
    putText(b, player, "Heggira");
    putText(b, player + 20, "Alkari");
    put8(b, player + 36, 0);
    put8(b, player + 37, 3);
    put16(b, player + 40, 0);
    put16(b, player + 49, 63);
    put8(b, player + 195 + 0, 3);
    put8(b, player + 195 + 4, 3);
    put8(b, player + 195 + 13, 2);
    put8(b, player + 278 + 2, 3);
    put16(b, player + 490, 25);
    put8(b, player + 800, 13);
    put8(b, player + 801, 3);
    put8(b, player + 2206, 2); // dictatorship
    putText(b, player + 805, "Scout");
    putText(b, player + 805 + 99, "Thrush");
    put8(b, player + 805 + 99 + 19, 1);
    put16(b, player + 805 + 99 + 28, 3); // a laser
    put8(b, player + 805 + 99 + 30, 2);
    put8(b, player + 805 + 99 + 32, 1);
    // Ships: a scout on its way to Derke, a colony ship queued at Sal and
    // the Guardian at Derke.
    put16(b, 139094, 3);
    std::size_t ship = 139096;
    putText(b, ship, "Scout");
    put8(b, ship + 99, 0);
    put8(b, ship + 100, 1);
    put16(b, ship + 101, 501);
    put16(b, ship + 103, 150);
    put16(b, ship + 105, 160);
    put8(b, ship + 108, 2);
    ship += 129;
    putText(b, ship, "Colony Ship");
    put8(b, ship + 17, 1);
    put8(b, ship + 99, 0);
    put8(b, ship + 100, 6);
    ship += 129;
    putText(b, ship, "Guardian");
    put8(b, ship + 16, 4);
    put8(b, ship + 99, 9);
    put16(b, ship + 101, 1);
    // The colony on the planet: five farmers, a worker, two scientists.
    const std::size_t colony = 603;
    put8(b, colony + 2, 0);
    put16(b, colony + 4, 0);
    put8(b, colony + 12, 8);
    for (int k = 0; k < 8; ++k) {
        const int job = k < 5 ? 0 : k < 6 ? 1 : 2;
        put16(b, colony + 14 + 4 * static_cast<std::size_t>(k), 0x211 + (job << 7));
    }
    put16(b, colony + 202, 73);
    put8(b, colony + 312 + 8, 1);  // capitol
    put8(b, colony + 312 + 24, 1); // marine barracks
    const int queue[] = {-3, 11, -101, -2, -7, -15, -1};
    for (std::size_t k = 0; k < 7; ++k) {
        put16(b, colony + 279 + 2 * k, queue[k] & 0xffff);
    }
    return b;
}

} // namespace

HGM_TEST("Saved games: the measured layout of fan patch 1.51") {
    const Parameters p = Parameters::defaults(kList);
    const TechTree tree(p);
    const SavedGame s = readSavegame(saved(), p, tree);
    HGM_CHECK(s.name == "Test Game");
    const Game& g = s.game;
    HGM_CHECK(g.stardate == 35012);
    HGM_CHECK(g.galaxy.width == 759 && g.galaxy.height == 600 && g.galaxy.mapSize == 1);
    HGM_CHECK(g.galaxy.stars.size() == 2 && g.galaxy.stars[0].name == "Sal" && g.galaxy.stars[1].name == "Derke");
    HGM_CHECK(g.galaxy.stars[0].x == 100 && g.galaxy.stars[0].y == 200 && g.galaxy.stars[0].size == 2);
    HGM_CHECK(g.galaxy.stars[0].colour == StarColour::Red && g.galaxy.stars[0].wormhole == -1);
    HGM_CHECK(g.galaxy.stars[0].orbits[1] == 0 && g.galaxy.stars[0].orbits[0] == kNoPlanet);
    HGM_CHECK(g.galaxy.nebulae.size() == 1 && g.galaxy.nebulae[0].picture == 7 && g.galaxy.nebulae[0].x == 324);
    const Planet& q = g.galaxy.planets.at(0);
    HGM_CHECK(q.star == 0 && q.orbit == 1 && q.type == SatelliteType::Planet && q.size == PlanetSize::Large);
    HGM_CHECK(q.climate == Climate::Ocean && q.minerals == Minerals::Poor && q.gravity == Gravity::Normal && q.image == 2);

    HGM_CHECK(g.players.size() == 1);
    const Player& pl = g.players.at(0);
    HGM_CHECK(pl.human && pl.race == 0 && pl.banner == 3 && pl.money == 63);
    HGM_CHECK(pl.picks[RacePicks::Government] == 2);
    HGM_CHECK(pl.knowledge.knowsField(0) && pl.knowledge.knowsField(4) && !pl.knowledge.knowsField(13));
    HGM_CHECK(pl.knowledge.knowsTech(2) && !pl.knowledge.knowsTech(3));
    HGM_CHECK(pl.research == 13 && pl.researchTech == 3 && pl.researched == 25);
    HGM_CHECK(pl.designs.size() == 6 && pl.designs[1].name == "Thrush" && pl.designs[1].weapons.size() == 1);
    HGM_CHECK(pl.designs[1].weapons[0].weapon == 3 && pl.designs[1].weapons[0].count == 2);
    HGM_CHECK(pl.explored.size() == 2 && pl.explored[0] && !pl.explored[1]);
    HGM_CHECK(g.galaxy.homes.size() == 1 && g.galaxy.homes[0].star == 0 && g.galaxy.homes[0].planet == 0);

    // The queued colony ship is in the build list, not in space; the
    // Guardian is a monster.
    HGM_CHECK(g.ships.size() == 1);
    const Ship& scout = g.ships.at(0);
    HGM_CHECK(scout.kind == ShipKind::Scout && scout.star == -1 && scout.destination == 1);
    HGM_CHECK(scout.x == 150 && scout.y == 160 && scout.speed == 2);
    HGM_CHECK(g.galaxy.monsters.size() == 1 && g.galaxy.monsters[0].kind == MonsterKind::Guardian &&
              g.galaxy.monsters[0].star == 1);

    HGM_CHECK(g.colonies.size() == 1);
    const Colony& c = g.colonies.at(0);
    HGM_CHECK(c.planet == 0 && c.jobs[0] == 5 && c.jobs[1] == 1 && c.jobs[2] == 2 && c.growth == 73);
    HGM_CHECK((c.buildings == std::vector<std::string>{"capitol", "marine_barracks"}));
    HGM_CHECK((c.buildList ==
               std::vector<std::string>{"housing", "colony_base", "colony_ship", "trade_goods", "spy", "freighters"}));
}

namespace {

bool refused(const std::vector<std::uint8_t>& b, const Parameters& p, const TechTree& tree) {
    try {
        readSavegame(b, p, tree);
    } catch (const FormatError&) {
        return true;
    }
    return false;
}

} // namespace

HGM_TEST("Saved games: damaged ones are refused") {
    const Parameters p = Parameters::defaults(kList);
    const TechTree tree(p);
    auto b = saved();
    b.pop_back();
    HGM_CHECK(refused(b, p, tree));
    b = saved();
    put8(b, 0, 0xd0);
    HGM_CHECK(refused(b, p, tree));
    b = saved();
    put16(b, 96977, 500); // too many stars
    HGM_CHECK(refused(b, p, tree));
    b = saved();
    put16(b, 96979 + 74 + 2, 40); // an orbit with a planet that isn't there
    HGM_CHECK(refused(b, p, tree));
    b = saved();
    put8(b, 90857 + 8, 12); // no such climate
    HGM_CHECK(refused(b, p, tree));
}
