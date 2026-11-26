// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/combat.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/races.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Combat: the manual's beam examples") {
    CombatWeapon deathRay;
    deathRay.minDamage = 50;
    deathRay.maxDamage = 100;
    // Death Ray at 23 squares, roll 85, attack 10 over defense: 18.
    BeamShot shot = beamShot(deathRay, 10, 23, 85);
    HGM_CHECK(shot.hit && shot.damage == 18);
    // At 11 squares: 35 + 36 * 25 / 30 = 65.
    shot = beamShot(deathRay, 10, 11, 85);
    HGM_CHECK(shot.hit && shot.damage == 65);
    // A lucky roll always hits, with the most damage.
    shot = beamShot(deathRay, -500, 23, 96);
    HGM_CHECK(shot.hit && shot.damage == 35);
    shot = beamShot(deathRay, -500, 23, 95);
    HGM_CHECK(!shot.hit);
    // Range penalties: heavy mounts halve the range, point defence doubles
    // it, the double range mod doubles the penalty.
    HGM_CHECK(rangePenalty(3, 0, 0) == 0 && rangePenalty(4, 0, 0) == 10 && rangePenalty(30, 0, 0) == 85);
    HGM_CHECK(rangePenalty(9, kModHeavyMount, 0) == 0 && rangePenalty(10, kModHeavyMount, 0) == 10);
    HGM_CHECK(rangePenalty(4, kModPointDefense, 0) == 30);
    HGM_CHECK(rangePenalty(7, 0, kNaturalDoubleRange) == 40);
}

HGM_TEST("Combat: shields, armour and structure") {
    Combatant c;
    c.shield = 3;
    c.armor = c.maxArmor = 4;
    c.structure = c.maxStructure = 4;
    takeHit(c, 2);
    HGM_CHECK(c.armor == 4 && c.structure == 4);
    takeHit(c, 8);
    HGM_CHECK(c.armor == 0 && c.structure == 3 && !c.destroyed);
    takeHit(c, 6);
    HGM_CHECK(c.structure == 0 && c.destroyed);
}

HGM_TEST("Combat: a scout that flies into a hydra's system dies") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    const Races races(data);
    Galaxy g;
    g.width = 759;
    g.height = 600;
    for (const char* name : {"Sol", "Ell"}) {
        Star s;
        s.name = name;
        s.x = g.stars.empty() ? 60 : 300;
        s.y = 60;
        g.stars.push_back(s);
    }
    Planet home;
    home.star = 0;
    home.orbit = 2;
    home.climate = Climate::Terran;
    home.size = PlanetSize::Medium;
    g.stars[0].orbits[2] = 0;
    g.planets.push_back(home);
    g.homes.push_back({0, 0, 0});
    g.monsters.push_back({MonsterKind::Hydra, 1});
    Game game = startGame(p, g, {races.stock(0)}, {0});

    // The measured hydra: structure 500, no armour, speed 8, three plasma
    // breaths.
    const Battle none = monsterBattle(game, p, 0);
    HGM_CHECK(none.units.size() == 1);
    const Combatant& hydra = none.units.front();
    HGM_CHECK(hydra.structure == 500 && hydra.armor == 0 && hydra.speed == 8 && hydra.hull == Hull::Cruiser);
    HGM_CHECK(hydra.weapons.size() == 1 && hydra.weapons.front().count == 3 && hydra.weapons.front().maxDamage == 60);

    const auto scout = std::find_if(game.ships.begin(), game.ships.end(),
                                    [](const Ship& s) { return s.kind == ShipKind::Scout; });
    HGM_CHECK(scout != game.ships.end());
    const std::size_t before = game.ships.size();
    scout->star = 1;
    const Battle fight = monsterBattle(game, p, 0);
    HGM_CHECK(fight.units.size() == 2);
    // The scout as the combat screen shows it: structure 4, armour 4.
    HGM_CHECK(fight.units[1].structure == 4 && fight.units[1].armor == 4 && fight.units[1].speed == 12);
    Random random(7);
    const auto reports = fightMonsters(game, p, random);
    HGM_CHECK(reports.size() == 1 && reports.front().player == 0 && reports.front().star == 1 &&
              reports.front().monster == MonsterKind::Hydra && reports.front().shipsLost == 1);
    HGM_CHECK(game.ships.size() == before - 1);
}
