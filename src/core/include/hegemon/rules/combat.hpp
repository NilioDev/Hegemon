// SPDX-License-Identifier: GPL-3.0-or-later
//
// Space combat: who fights at the end of a turn, the ships and monsters in
// a battle, and how a battle plays out when the captains fight it on their
// own (the combat screen's AUTO). The beam formulas are the ones the fan
// patch's manual gives; the rest is described in docs/rules/combat.md,
// with what is measured and what is assumed.
#pragma once

#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/galaxy.hpp"
#include "hegemon/rules/ship_design.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hegemon {

class Parameters;
class Random;

// The last of a range's three squares: a beam's accuracy and damage
// change with the range, one range for every three squares.
inline constexpr int kSquaresARange = 3;

// One weapon mount as it fights.
struct CombatWeapon {
    int weapon = 0; // the weapon table's number
    int weaponClass = 0;
    int count = 0;
    int mods = 0;        // as WeaponMount::mods
    int natural = 0;     // the weapon's own mods: kNaturalDoubleRange and so on
    int minDamage = 0, maxDamage = 0;
    int ammo = -1;       // shots left of a rack, -1 for beams
};
// The weapon table's natural_mods that the beam formulas use.
inline constexpr int kNaturalDoubleRange = 2, kNaturalNoDissipation = 4;

// A ship, monster or orbital base in a battle.
struct Combatant {
    enum class Kind : std::uint8_t { Ship, Monster };
    Kind kind = Kind::Ship;
    int owner = -1;    // the player, -1 for a monster
    int ship = -1;     // the game's ship, for a Ship
    int monster = -1;  // the galaxy's monster, for a Monster
    std::string name;
    Hull hull = Hull::Frigate;
    int structure = 0, maxStructure = 0;
    int armor = 0, maxArmor = 0;
    int shield = 0; // damage each hit loses to the shields
    int beamAttack = 0, beamDefense = 0;
    int speed = 0; // movement points a combat turn, a square each
    std::vector<CombatWeapon> weapons;
    bool armed = false; // carries a weapon at all
    int x = 0, y = 0;   // in squares
    bool destroyed = false;
};

struct Battle {
    int star = 0;
    std::vector<Combatant> units;
    int rounds = 0;
    // The side that won: a player, -1 for the monsters, nullopt for none.
    std::optional<int> winner;
};

// The battle field, in squares (assumed; see docs/rules/combat.md).
inline constexpr int kFieldWidth = 80, kFieldHeight = 52;

// Whether a beam hits and what it does: the fan patch manual's classic
// formula. `attack` is the shooter's beam attack less the target's beam
// defense; `squares` the distance; `roll` a number from 1 to 100.
struct BeamShot {
    bool hit = false;
    int damage = 0;
};
int rangePenalty(int squares, int mods, int natural);
BeamShot beamShot(const CombatWeapon& weapon, int attack, int squares, int roll);

// What `damage` does to a combatant: its shields take their part of the
// hit, its armour what it can hold, its structure the rest.
void takeHit(Combatant& target, int damage);

// The combatants of a monster's battle at `star`: the monster and every
// ship of the players there.
Battle monsterBattle(const Game& game, const Parameters& parameters, int monster);

// The captains fight it out: in each combat turn every unit, fastest first,
// moves towards the nearest enemy and fires all it can at it, until one
// side is gone or combat_turns end (max_combat_turns).
void fightAuto(Battle& battle, const Parameters& parameters, Random& random);

// The battles of the turn: each monster fights the ships of the players at
// its star. Ships destroyed leave the game. Returns what the players are
// told, the human's first.
std::vector<BattleReport> fightMonsters(Game& game, const Parameters& parameters, Random& random);

} // namespace hegemon
