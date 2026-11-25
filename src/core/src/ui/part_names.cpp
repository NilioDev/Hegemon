// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/part_names.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/rules/tech.hpp"

namespace hegemon {

// The ship parts' names at the end of TECHNAME.LBX's first entry (counted
// back from its last name, as the buildings before them are more than the
// fan patch's table has): "No Special" and the specials, the armors,
// shields, weapons, computers and drives (each list starting with its
// "No ..."), 4 kinds of units, 6 fuel cells, the mods' long and short names
// (each after a name for none), the weapons' plurals, then the hull sizes'
// names and their plurals.
PartNames partNames(const Parameters& p, const TechNames& names) {
    const auto rows = [&](std::string_view setting) {
        const ParameterDefinition* d = p.definition(setting);
        if (d == nullptr || d->rowNames.empty()) {
            throw FormatError("missing setting " + std::string(setting));
        }
        return d->rowNames[0].size() + 1;
    };
    constexpr std::size_t kUnits = 4, kFuelCells = 6, kHullNames = 9;
    std::size_t last = 0;
    for (std::size_t i = 0; const std::string* w = names.afterTechs(i); ++i) {
        if (!w->empty()) {
            last = i;
        }
    }
    const std::size_t total = rows("special") + rows("armor") + rows("shield") + rows("weapon") + rows("computer") +
                              rows("drive") + kUnits + kFuelCells + 2 * rows("weapon_mod") + rows("weapon") +
                              2 * kHullNames;
    if (last + 1 < total) {
        throw FormatError("TECHNAME.LBX: too few names");
    }
    std::size_t at = last + 1 - total;
    const auto take = [&](std::size_t n) {
        std::vector<std::string> list;
        for (std::size_t i = 0; i < n; ++i) {
            list.push_back(*names.afterTechs(at + i));
        }
        at += n;
        return list;
    };
    PartNames n;
    n.specials = take(rows("special"));
    n.armor = take(rows("armor"));
    n.shields = take(rows("shield"));
    n.weapons = take(rows("weapon"));
    n.computers = take(rows("computer"));
    n.drives = take(rows("drive"));
    at += kUnits + kFuelCells + rows("weapon_mod");
    n.shortMods = take(rows("weapon_mod"));
    n.plurals = take(rows("weapon"));
    n.hulls = take(kHullNames);
    n.hullPlurals = take(kHullNames);
    return n;
}

const std::string& nameIn(const std::vector<std::string>& list, int index) {
    static const std::string none;
    return index >= 0 && static_cast<std::size_t>(index) < list.size() ? list[static_cast<std::size_t>(index)] : none;
}

std::string mountText(const WeaponMount& mount, const PartNames& names) {
    std::string text = std::to_string(mount.count) + " ";
    for (std::size_t bit = 1; bit < names.shortMods.size(); ++bit) {
        if (((mount.mods >> bit) & 1) != 0) {
            text += names.shortMods[bit] + " ";
        }
    }
    return text + nameIn(mount.count == 1 ? names.weapons : names.plurals, mount.weapon);
}

} // namespace hegemon
