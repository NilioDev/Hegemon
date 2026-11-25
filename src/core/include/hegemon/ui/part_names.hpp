// SPDX-License-Identifier: GPL-3.0-or-later
//
// The names of ship parts and hull sizes at the end of TECHNAME.LBX's first
// entry, and the words the original writes for a design's weapons. Used by
// the build list's design box and the Fleets screen.
#pragma once

#include "hegemon/rules/ship_design.hpp"

#include <string>
#include <vector>

namespace hegemon {

class Parameters;
class TechNames;

// Each list starts with its name for none ("No Shield"). The hull sizes
// are the hull table's nine, from frigate to star fortress.
struct PartNames {
    std::vector<std::string> specials, armor, shields, weapons, computers, drives, shortMods, plurals;
    std::vector<std::string> hulls, hullPlurals;
};

// Throws FormatError when the entry has too few names, std::out_of_range
// when a setting is missing.
PartNames partNames(const Parameters& parameters, const TechNames& names);

// The name at `index`, or an empty one.
const std::string& nameIn(const std::vector<std::string>& list, int index);

// A mount as the design box writes it: "1 Laser Cannon", the count, the
// mods' short names and the weapon's name (its plural for more than one).
std::string mountText(const WeaponMount& mount, const PartNames& names);

} // namespace hegemon
