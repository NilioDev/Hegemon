// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/ship_design.hpp"
#include "hegemon/rules/tech.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

constexpr int kLaser = 3, kNuclearMissile = 14, kNuclearBomb = 21;

ShipDesign design(Hull hull, std::vector<WeaponMount> weapons) {
    ShipDesign d;
    d.hull = hull;
    d.computer = 1;
    d.drive = 1;
    d.armor = 1;
    d.weapons = std::move(weapons);
    return d;
}

} // namespace

HGM_TEST("Ship designs: costs and combat speeds of measured starting designs") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    const TechTree tree(p);
    const ShipParts parts(p, tree);
    const Knowledge k = tree.averageStart();
    struct Case {
        ShipDesign design;
        int cost, speed;
    };
    ShipDesign scout = design(Hull::Frigate, {{kLaser, 1, kArcForward, 0, 0}});
    scout.specials = std::uint64_t{1} << 11;
    const WeaponMount bomb{kNuclearBomb, 1, kArcAll, 0, 10};
    const auto bombs = [&](int n) {
        WeaponMount m = bomb;
        m.count = n;
        return m;
    };
    const auto lasers = [&](int n, int arc, int mods) { return WeaponMount{kLaser, n, arc, mods, 0}; };
    const auto missiles = [&](int n, int shots) { return WeaponMount{kNuclearMissile, n, kArcAll, 0, shots}; };
    // Measured in saved games of the original (1.51, average technology).
    const std::vector<Case> cases = {
        {scout, 35, 12},
        {design(Hull::Frigate, {lasers(2, kArcForward, 0), bombs(1)}), 37, 11},
        {design(Hull::Frigate, {lasers(4, kArcForward, kModPointDefense), bombs(1)}), 35, 11},
        {design(Hull::Destroyer, {lasers(1, kArcForward, kModHeavyMount), lasers(3, kArcForward, 0), bombs(2)}), 114,
         9},
        {design(Hull::Destroyer,
                {lasers(1, kArcForward, kModHeavyMount), lasers(2, kArcForwardExtended, 0), bombs(4)}),
         116, 8},
        {design(Hull::Destroyer, {missiles(1, 5), lasers(1, kArcForward, kModHeavyMount), lasers(1, kArcForward, 0),
                                  bombs(2)}),
         124, 9},
        {design(Hull::Cruiser, {missiles(3, 5), lasers(1, kArcForward, kModHeavyMount),
                                lasers(2, kArcForwardExtended, 0), bombs(4)}),
         391, 6},
        {design(Hull::Cruiser, {missiles(3, 10), lasers(3, kArcForward, kModPointDefense), bombs(3)}), 402, 7},
        {design(Hull::Cruiser, {missiles(3, 2), lasers(1, kArcForwardExtended, kModHeavyMount),
                                lasers(4, kArcForward, 0), bombs(6)}),
         375, 7},
        {design(Hull::Battleship, {missiles(5, 10), lasers(4, kArcForwardExtended, 0),
                                   lasers(4, kArcForwardExtended, kModPointDefense), bombs(6)}),
         920, 6},
        {design(Hull::Titan, {missiles(11, 10), lasers(7, kArcForward, 0), lasers(7, kArcAll, kModPointDefense),
                              bombs(12)}),
         2213, 5},
        {design(Hull::Titan, {missiles(20, 2), lasers(6, kArcForward, kModHeavyMount), lasers(10, kArcForward, 0),
                              bombs(20)}),
         2155, 4},
    };
    for (const Case& c : cases) {
        HGM_CHECK(parts.cost(c.design, k) == c.cost);
        HGM_CHECK(parts.combatSpeed(c.design, k) == c.speed);
    }
    // Cold fusion makes the nuclear bomb one level smaller.
    HGM_CHECK(parts.miniaturization(k, static_cast<int>(p.integer("tech", {"nuclear_bomb"}, "id"))) == 1);
    HGM_CHECK(parts.miniaturization(k, static_cast<int>(p.integer("tech", {"laser_cannon"}, "id"))) == 0);
}

HGM_TEST("Ship designs: the auto-designer fills the tables as measured") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    const TechTree tree(p);
    const ShipParts parts(p, tree);
    const Knowledge k = tree.averageStart();
    const auto weapons = [&](Hull hull, std::size_t table, int shots) {
        return autoDesign(p, parts, k, hull, table, shots, nullptr).weapons;
    };
    using M = std::vector<WeaponMount>;
    // Measured designs of the standard_beam_missile_x2 table (all forward).
    HGM_CHECK((weapons(Hull::Frigate, 0, 2) == M{{kLaser, 2, 1, 0, 0}, {kNuclearBomb, 1, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Destroyer, 0, 2) ==
               M{{kLaser, 1, 1, kModHeavyMount, 0}, {kLaser, 3, 1, 0, 0}, {kNuclearBomb, 2, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Cruiser, 0, 2) == M{{kNuclearMissile, 3, kArcAll, 0, 2},
                                                  {kLaser, 1, 1, kModHeavyMount, 0},
                                                  {kLaser, 5, 1, 0, 0},
                                                  {kNuclearBomb, 5, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Battleship, 0, 2) == M{{kNuclearMissile, 8, kArcAll, 0, 2},
                                                     {kLaser, 3, 1, kModHeavyMount, 0},
                                                     {kLaser, 7, 1, 0, 0},
                                                     {kNuclearBomb, 9, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Titan, 0, 2) == M{{kNuclearMissile, 20, kArcAll, 0, 2},
                                                {kLaser, 6, 1, kModHeavyMount, 0},
                                                {kLaser, 10, 1, 0, 0},
                                                {kNuclearBomb, 20, kArcAll, 0, 10}}));
    // hybrid_beam_missile_x5 and standard_missile_x10.
    HGM_CHECK((weapons(Hull::Destroyer, 2, 5) == M{{kNuclearMissile, 1, kArcAll, 0, 5},
                                                    {kLaser, 1, 1, kModHeavyMount, 0},
                                                    {kLaser, 1, 1, 0, 0},
                                                    {kNuclearBomb, 2, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Titan, 2, 5) == M{{kNuclearMissile, 15, kArcAll, 0, 5},
                                                {kLaser, 3, 1, kModHeavyMount, 0},
                                                {kLaser, 9, 1, 0, 0},
                                                {kNuclearBomb, 12, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Frigate, 3, 10) ==
               M{{kLaser, 4, 1, kModPointDefense, 0}, {kNuclearBomb, 1, kArcAll, 0, 10}}));
    HGM_CHECK((weapons(Hull::Destroyer, 3, 10) == M{{kNuclearMissile, 1, kArcAll, 0, 10},
                                                     {kLaser, 4, 1, kModPointDefense, 0},
                                                     {kNuclearBomb, 2, kArcAll, 0, 10}}));

    // The starting designs: the Scout first, then one of each hull to the
    // titan, each named from its hull's eight names.
    Random random(7);
    const auto designs = startingDesigns(p, parts, k, random);
    HGM_CHECK(designs.size() == kDesignSlots);
    HGM_CHECK(designs[0].nameIndex == -1 && designs[0].cost == 35 && designs[0].combatSpeed == 12);
    for (std::size_t i = 1; i < designs.size(); ++i) {
        HGM_CHECK(designs[i].hull == static_cast<Hull>(i - 1));
        HGM_CHECK(designs[i].nameIndex / 8 == static_cast<int>(i - 1) && designs[i].image == designs[i].nameIndex);
        HGM_CHECK(designs[i].cost > 0);
    }
}
