// SPDX-License-Identifier: GPL-3.0-or-later
//
// Ship designs: what a design carries, what it costs, how much room its
// parts take and how fast it moves in combat, read from the fan patch's
// rule settings (hull, weapon, weapon_mod, missile_space_and_cost, special,
// computer, shield, armor, drive). Also the designs a player starts a game
// with, made the way the original's auto-designer makes them from the
// ai_ship_design tables. See docs/rules/ship-design.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace hegemon {

class Parameters;
class Random;
class TechTree;
struct Knowledge;

// Hull sizes, in the order of the settings' hull table.
enum class Hull : std::uint8_t { Frigate, Destroyer, Cruiser, Battleship, Titan, DoomStar };
inline constexpr std::size_t kHullSizes = 6;

// Firing arcs, as the original stores them. Missiles and bombs are stored
// with All.
inline constexpr int kArcForward = 1, kArcForwardExtended = 2, kArcBack = 4, kArcBackExtended = 8, kArcAll = 16;
// Weapon mods: bit n is the weapon_mod table's mod number n.
inline constexpr int kModHeavyMount = 1 << 1, kModPointDefense = 1 << 2;

struct WeaponMount {
    int weapon = 0; // the weapon table's number
    int count = 0;
    int arc = kArcForward;
    int mods = 0;
    int ammo = 0; // shots of a missile rack or bomb bay
    bool operator==(const WeaponMount&) const = default;
};

struct ShipDesign {
    // The name: a custom one, or empty for the race's name number
    // `nameIndex` (SHIPNAME.LBX, 8 a hull size); -1 for the Scout.
    std::string name;
    int nameIndex = -1;
    Hull hull = Hull::Frigate;
    int shield = 0;   // 0 for none, else the shield table's row + 1
    int drive = 1;    // the drive table's row + 1
    int computer = 0; // 0 for none, else the computer table's row + 1
    int armor = 1;    // the armor table's row + 1
    std::uint64_t specials = 0; // bit n: the special table's special n
    std::vector<WeaponMount> weapons;
    int image = 0; // the picture: the number of the name it was made with
    int cost = 0;
    int combatSpeed = 0;

    bool hasSpecial(int id) const { return id > 0 && id < 64 && ((specials >> id) & 1) != 0; }
};

// The parts a design is made from, and their costs and room for a player.
class ShipParts {
public:
    // Throws std::out_of_range when a setting is missing.
    ShipParts(const Parameters& parameters, const TechTree& tree);

    // Rows of the weapon table.
    struct Weapon {
        int number = 0;
        int tech = 0;
        int weaponClass = 0; // 0 beam, 1 missile, 2 torpedo, 3 bomb, 4 fighters, 5 special
        int ammo = 0;
        int space = 0;
        int cost = 0;
        std::string mods; // the mods it may have, such as "hv,pd,ap,af"
    };
    const std::vector<Weapon>& weapons() const { return weapons_; }
    const Weapon* weapon(int number) const;

    int hullSpace(Hull hull) const;
    int hullCost(Hull hull) const;

    // How many levels of miniaturization a player has for a part of the
    // tech `tech`: the fields after the tech's own in its category that
    // the player knows.
    int miniaturization(const Knowledge& knowledge, int tech) const;

    // One weapon of a mount: its cost and the room it takes, after
    // miniaturization, mods and arc.
    double weaponCost(const Knowledge& knowledge, const WeaponMount& mount) const;
    double weaponSpace(const Knowledge& knowledge, const WeaponMount& mount) const;
    double specialCost(const Knowledge& knowledge, int special, Hull hull) const;
    double specialSpace(const Knowledge& knowledge, int special, Hull hull) const;
    int specialTech(int special) const;
    int shieldSpace(int shield, Hull hull) const;

    // The whole design: its cost, the room it uses and its combat speed.
    int cost(const ShipDesign& design, const Knowledge& knowledge) const;
    double spaceUsed(const ShipDesign& design, const Knowledge& knowledge) const;
    int combatSpeed(const ShipDesign& design, const Knowledge& knowledge) const;
    // Sets the design's cost and combat speed.
    void finish(ShipDesign& design, const Knowledge& knowledge) const;

    // The best part of each kind the player knows (0 for none): the last
    // known row of the table.
    int bestComputer(const Knowledge& knowledge) const;
    int bestDrive(const Knowledge& knowledge) const;
    int bestArmor(const Knowledge& knowledge) const;
    int bestShield(const Knowledge& knowledge) const;

private:
    const Parameters& p_;
    const TechTree& tree_;
    std::vector<Weapon> weapons_;
    std::vector<int> computerTechs_, driveTechs_, armorTechs_, shieldTechs_;
};

// What the auto-designer makes of a hull with the ai_ship_design table
// number `table`, its missiles racks of `shots`. Beam groups get their arcs
// from `random`; all forward without one.
ShipDesign autoDesign(const Parameters& parameters, const ShipParts& parts, const Knowledge& knowledge, Hull hull,
                      std::size_t table, int shots, Random* random);

// The slots of designs a player has.
inline constexpr std::size_t kDesignSlots = 6;

// The designs a player starts with: a Scout, then a frigate, a destroyer, a
// cruiser, a battleship and a titan made by the auto-designer.
std::vector<ShipDesign> startingDesigns(const Parameters& parameters, const ShipParts& parts,
                                        const Knowledge& knowledge, Random& random);

} // namespace hegemon
