// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/ship_design.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/tech.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace hegemon {

namespace {

constexpr std::array<std::string_view, kHullSizes> kHullNames = {"frigate",    "destroyer", "cruiser",
                                                                  "battleship", "titan",     "doomstar"};
constexpr std::array<std::string_view, kHullSizes> kSizeSuffix = {"ff", "dd", "ca", "bb", "tt", "ds"};

// Miniaturization, measured at one level only (a nuclear bomb with cold
// fusion known costs 2.25 and takes 4 of 5): a quarter less a level. The
// levels after the first are assumed to take a quarter of what is left, up
// to five levels.
constexpr int kMaxMiniaturization = 5;
constexpr double kMiniaturizationStep = 0.75;
// Measured: a mod or an arc rounds the cost and room of one weapon down.
// A forward-extended or backward-extended arc adds a quarter, a 360 degree
// arc a half (both only for beams).
constexpr double kExtendedArc = 1.25, kAllArc = 1.5;
// Specials whose room doesn't shrink with miniaturization (extended fuel
// tanks, reinforced hull, heavy armor, battle pods, troop pods, augmented
// engines), and the one whose cost doesn't either (battle pods).
constexpr std::array<int, 6> kFixedRoom = {11, 27, 14, 4, 36, 2};
constexpr int kBattlePods = 4;
constexpr int kExtendedFuelTanks = 11;
// The missile racks of the missile_space_and_cost table, by shots.
constexpr std::array<int, 5> kRackShots = {2, 5, 10, 15, 20};
// Weapon classes.
constexpr int kBeam = 0, kMissile = 1, kTorpedo = 2, kBomb = 3, kFighter = 4;

std::size_t columnOf(const Parameters& p, std::string_view name, std::string_view column) {
    const ParameterDefinition* d = p.definition(name);
    for (std::size_t i = 0; d != nullptr && i < d->columns.size(); ++i) {
        if (d->columns[i] == column) {
            return i;
        }
    }
    throw std::out_of_range("setting " + std::string(name) + " has no column " + std::string(column));
}

const std::vector<std::string>& rowNames(const Parameters& p, std::string_view name, std::size_t dimension = 0) {
    const ParameterDefinition* d = p.definition(name);
    if (d == nullptr || d->rowNames.size() <= dimension) {
        throw std::out_of_range("setting " + std::string(name) + " missing or without named rows");
    }
    return d->rowNames[dimension];
}

int cell(const Parameters& p, std::string_view name, std::size_t row, std::string_view column) {
    return static_cast<int>(p.integer(name, row, columnOf(p, name, column)));
}

double miniaturized(double value, int level) {
    for (int i = 0; i < level; ++i) {
        value *= kMiniaturizationStep;
    }
    return value;
}

std::string sized(std::string_view prefix, Hull hull, std::string_view suffix = {}) {
    std::string s(prefix);
    s += kSizeSuffix[static_cast<std::size_t>(hull)];
    s += suffix;
    return s;
}

bool allows(const std::string& mods, std::string_view mod) {
    std::size_t at = 0;
    while (at <= mods.size()) {
        const std::size_t end = std::min(mods.find(',', at), mods.size());
        if (std::string_view(mods).substr(at, end - at) == mod) {
            return true;
        }
        at = end + 1;
    }
    return false;
}

} // namespace

ShipParts::ShipParts(const Parameters& parameters, const TechTree& tree) : p_(parameters), tree_(tree) {
    const auto& names = rowNames(p_, "weapon");
    const std::size_t modsColumn = columnOf(p_, "weapon", "available_mods");
    for (std::size_t r = 0; r < names.size(); ++r) {
        Weapon w;
        w.number = cell(p_, "weapon", r, "number");
        w.tech = cell(p_, "weapon", r, "tech_id");
        w.weaponClass = cell(p_, "weapon", r, "class");
        w.ammo = cell(p_, "weapon", r, "ammo");
        w.space = cell(p_, "weapon", r, "size");
        w.cost = cell(p_, "weapon", r, "cost");
        w.mods = p_.value("weapon", r, modsColumn).text;
        weapons_.push_back(std::move(w));
    }
    const auto techOf = [&](std::string_view name) {
        return static_cast<int>(p_.integer("tech", {name}, "id"));
    };
    for (const std::string& n : rowNames(p_, "computer")) {
        computerTechs_.push_back(techOf(n));
    }
    for (const std::string& n : rowNames(p_, "drive")) {
        driveTechs_.push_back(techOf(n));
    }
    for (const std::string& n : rowNames(p_, "armor")) {
        armorTechs_.push_back(techOf(n + "_armor"));
    }
    for (const std::string& n : rowNames(p_, "shield")) {
        shieldTechs_.push_back(techOf(n));
    }
}

const ShipParts::Weapon* ShipParts::weapon(int number) const {
    for (const Weapon& w : weapons_) {
        if (w.number == number) {
            return &w;
        }
    }
    return nullptr;
}

int ShipParts::hullSpace(Hull hull) const {
    return static_cast<int>(p_.integer("hull", {kHullNames[static_cast<std::size_t>(hull)]}, "size"));
}

int ShipParts::hullCost(Hull hull) const {
    return static_cast<int>(p_.integer("hull", {kHullNames[static_cast<std::size_t>(hull)]}, "cost"));
}

int ShipParts::miniaturization(const Knowledge& knowledge, int tech) const {
    const int field = tree_.techField(tech);
    if (field < 0) {
        return 0;
    }
    int level = 0;
    int at = field;
    while (level < kMaxMiniaturization) {
        const int next = tree_.field(at).next;
        if (next == 0 || next == at || !knowledge.knowsField(next)) {
            break;
        }
        ++level;
        at = next;
    }
    return level;
}

namespace {

// The missile table's row for a rack of `shots`, or the last one.
std::size_t rackRow(int shots) {
    for (std::size_t i = 0; i < kRackShots.size(); ++i) {
        if (shots <= kRackShots[i]) {
            return i;
        }
    }
    return kRackShots.size() - 1;
}

} // namespace

double ShipParts::weaponCost(const Knowledge& knowledge, const WeaponMount& mount) const {
    const Weapon* w = weapon(mount.weapon);
    if (w == nullptr) {
        return 0;
    }
    double unit = w->cost;
    if (w->weaponClass == kMissile) {
        unit = cell(p_, "missile_space_and_cost", rackRow(mount.ammo), "cost");
    }
    unit = miniaturized(unit, miniaturization(knowledge, w->tech));
    const auto& mods = rowNames(p_, "weapon_mod");
    for (std::size_t m = 0; m < mods.size(); ++m) {
        if (((mount.mods >> (m + 1)) & 1) != 0) {
            unit = std::floor(unit * (100 + cell(p_, "weapon_mod", m, "cost")) / 100.0);
        }
    }
    if (w->weaponClass == kBeam) {
        if ((mount.arc & kArcAll) != 0 || mount.arc == 15) {
            unit = std::floor(unit * kAllArc);
        } else if ((mount.arc & (kArcForwardExtended | kArcBackExtended)) != 0) {
            unit = std::floor(unit * kExtendedArc);
        }
    }
    return unit;
}

double ShipParts::weaponSpace(const Knowledge& knowledge, const WeaponMount& mount) const {
    const Weapon* w = weapon(mount.weapon);
    if (w == nullptr) {
        return 0;
    }
    double unit = w->space;
    if (w->weaponClass == kMissile) {
        unit = cell(p_, "missile_space_and_cost", rackRow(mount.ammo), "space");
    }
    const int level = miniaturization(knowledge, w->tech);
    if (level > 0) {
        // Measured: a nuclear bomb's 3.75 takes 4.
        unit = std::floor(miniaturized(unit, level) + 0.5);
    }
    const auto& mods = rowNames(p_, "weapon_mod");
    for (std::size_t m = 0; m < mods.size(); ++m) {
        if (((mount.mods >> (m + 1)) & 1) != 0) {
            unit = std::floor(unit * (100 + cell(p_, "weapon_mod", m, "space")) / 100.0);
        }
    }
    if (w->weaponClass == kBeam) {
        if ((mount.arc & kArcAll) != 0 || mount.arc == 15) {
            unit = std::floor(unit * kAllArc);
        } else if ((mount.arc & (kArcForwardExtended | kArcBackExtended)) != 0) {
            unit = std::floor(unit * kExtendedArc);
        }
    }
    return unit;
}

int ShipParts::specialTech(int special) const {
    const auto& names = rowNames(p_, "special");
    if (special < 1 || static_cast<std::size_t>(special) > names.size()) {
        return 0;
    }
    return cell(p_, "special", static_cast<std::size_t>(special - 1), "tech_id");
}

double ShipParts::specialCost(const Knowledge& knowledge, int special, Hull hull) const {
    const auto& names = rowNames(p_, "special");
    if (special < 1 || static_cast<std::size_t>(special) > names.size()) {
        return 0;
    }
    const auto row = static_cast<std::size_t>(special - 1);
    double cost = cell(p_, "special", row, "cost" + std::to_string(static_cast<int>(hull) + 1));
    if (special != kBattlePods) {
        cost = miniaturized(cost, miniaturization(knowledge, specialTech(special)));
    }
    return cost;
}

double ShipParts::specialSpace(const Knowledge& knowledge, int special, Hull hull) const {
    const auto& names = rowNames(p_, "special");
    if (special < 1 || static_cast<std::size_t>(special) > names.size()) {
        return 0;
    }
    const auto row = static_cast<std::size_t>(special - 1);
    double space = cell(p_, "special", row, "size" + std::to_string(static_cast<int>(hull) + 1));
    const int level = miniaturization(knowledge, specialTech(special));
    if (level > 0 && std::find(kFixedRoom.begin(), kFixedRoom.end(), special) == kFixedRoom.end()) {
        space = std::floor(miniaturized(space, level) + 0.5);
    }
    return space;
}

int ShipParts::shieldSpace(int shield, Hull hull) const {
    if (shield <= 0 || static_cast<std::size_t>(shield) > shieldTechs_.size()) {
        return 0;
    }
    return cell(p_, "shield", static_cast<std::size_t>(shield - 1), sized("space_", hull));
}

double ShipParts::spaceUsed(const ShipDesign& d, const Knowledge& knowledge) const {
    double used = shieldSpace(d.shield, d.hull);
    for (const WeaponMount& m : d.weapons) {
        used += m.count * weaponSpace(knowledge, m);
    }
    for (int s = 1; s < 64; ++s) {
        if (d.hasSpecial(s)) {
            used += specialSpace(knowledge, s, d.hull);
        }
    }
    return used;
}

int ShipParts::cost(const ShipDesign& d, const Knowledge& knowledge) const {
    double total = hullCost(d.hull);
    if (d.computer > 0 && static_cast<std::size_t>(d.computer) <= computerTechs_.size()) {
        total += cell(p_, "computer", static_cast<std::size_t>(d.computer - 1), sized("cost_", d.hull));
    }
    if (d.shield > 0 && static_cast<std::size_t>(d.shield) <= shieldTechs_.size()) {
        total += cell(p_, "shield", static_cast<std::size_t>(d.shield - 1), sized("cost_", d.hull));
    }
    if (d.drive > 0 && static_cast<std::size_t>(d.drive) <= driveTechs_.size()) {
        total += cell(p_, "drive", static_cast<std::size_t>(d.drive - 1), sized("", d.hull, "_c"));
    }
    if (d.armor > 0 && static_cast<std::size_t>(d.armor) <= armorTechs_.size()) {
        total += hullCost(d.hull) * cell(p_, "armor", static_cast<std::size_t>(d.armor - 1), "cost%") / 100.0;
    }
    for (const WeaponMount& m : d.weapons) {
        total += m.count * weaponCost(knowledge, m);
    }
    for (int s = 1; s < 64; ++s) {
        if (d.hasSpecial(s)) {
            total += specialCost(knowledge, s, d.hull);
        }
    }
    return static_cast<int>(std::floor(total + 1e-9));
}

int ShipParts::combatSpeed(const ShipDesign& d, const Knowledge& knowledge) const {
    if (d.drive <= 0 || static_cast<std::size_t>(d.drive) > driveTechs_.size()) {
        return 0;
    }
    const auto row = static_cast<std::size_t>(d.drive - 1);
    const int low = cell(p_, "drive", row, sized("", d.hull, "_min"));
    const int high = cell(p_, "drive", row, sized("", d.hull, "_max"));
    const double space = hullSpace(d.hull);
    const double free = space - spaceUsed(d, knowledge);
    // Measured: empty room adds to the slowest speed, rounded up.
    const int bonus = space > 0 ? static_cast<int>(std::ceil((high - low) * free / space - 1e-9)) : 0;
    return low + std::max(0, bonus);
}

void ShipParts::finish(ShipDesign& d, const Knowledge& knowledge) const {
    d.cost = cost(d, knowledge);
    d.combatSpeed = combatSpeed(d, knowledge);
}

namespace {

int lastKnown(const std::vector<int>& techs, const Knowledge& knowledge) {
    int best = 0;
    for (std::size_t i = 0; i < techs.size(); ++i) {
        if (knowledge.knowsTech(techs[i])) {
            best = static_cast<int>(i) + 1;
        }
    }
    return best;
}

} // namespace

int ShipParts::bestComputer(const Knowledge& k) const {
    return lastKnown(computerTechs_, k);
}

int ShipParts::bestDrive(const Knowledge& k) const {
    return std::max(1, lastKnown(driveTechs_, k));
}

int ShipParts::bestArmor(const Knowledge& k) const {
    return std::max(1, lastKnown(armorTechs_, k));
}

int ShipParts::bestShield(const Knowledge& k) const {
    return lastKnown(shieldTechs_, k);
}

namespace {

// The auto-designer's columns, in the order it fills them.
enum Column : std::uint8_t {
    ThemeSpecial,
    BeamSpecial,
    MissileSpecial,
    DefenseSpecial,
    SpecialWeapon,
    SpecialDevice,
    Fighter,
    Missiles,
    HeavyBeams,
    Beams,
    PointDefense,
    Bombs,
    kColumns
};
constexpr std::array<std::string_view, kColumns> kColumnNames = {
    "theme_special", "beam_special", "missile_special", "defense_special", "special_weapon", "special_device",
    "fighter",       "missile",      "heavy_beams",     "beams",           "point_defense_beams", "bombs"};

// Measured on 252 starting designs: the tables the designs come from, as
// the missiles they carry tell (x2 for standard_beam_missile_x2, x5 for
// hybrid_beam_missile_x5, x10 for standard_missile_x10), about 60, 32 and 8
// in a hundred. The tables that need fighters, special weapons or devices
// are never used at the start.
constexpr std::array<std::size_t, 3> kStartTables = {0, 2, 3};
constexpr std::array<std::int64_t, 3> kStartTableWeights = {60, 32, 8};
constexpr std::array<int, 3> kStartTableShots = {2, 5, 10};
// Measured: a beam group gets a forward-extended arc about 10 times in a
// hundred a hull size (none on frigates, about a third on titans).
constexpr int kExtendedArcChance = 10;
// Special lists the auto-designer picks from, by column.
constexpr std::string_view kBeamSpecials = "ai_ship_design_beam_offense_specials";
constexpr std::string_view kMissileSpecials = "ai_ship_design_missile_offense_specials";
constexpr std::string_view kDefenseSpecials = "ai_ship_design_defense_specials_a";
constexpr std::string_view kDeviceSpecials = "ai_ship_design_special_device_specials";

// The last weapon in the table of one of `classes` that the player knows,
// and that may carry `mod` when one is given.
const ShipParts::Weapon* bestWeapon(const ShipParts& parts, const Knowledge& k, std::initializer_list<int> classes,
                                    std::string_view mod = {}) {
    const ShipParts::Weapon* best = nullptr;
    for (const ShipParts::Weapon& w : parts.weapons()) {
        if (std::find(classes.begin(), classes.end(), w.weaponClass) == classes.end() || !k.knowsTech(w.tech)) {
            continue;
        }
        // Bio-weapons only come with a theme.
        if (w.weaponClass == kBomb && (w.number == 25 || w.number == 26)) {
            continue;
        }
        if (!mod.empty() && !allows(w.mods, mod)) {
            continue;
        }
        best = &w;
    }
    return best;
}

} // namespace

ShipDesign autoDesign(const Parameters& p, const ShipParts& parts, const Knowledge& k, Hull hull, std::size_t table,
                      int shots, Random* random) {
    ShipDesign d;
    d.hull = hull;
    d.computer = parts.bestComputer(k);
    d.drive = parts.bestDrive(k);
    d.armor = parts.bestArmor(k);
    d.shield = parts.bestShield(k);
    const double space = parts.hullSpace(hull) - parts.shieldSpace(d.shield, hull);
    const auto& tables = rowNames(p, "ai_ship_design", 0);
    const std::string& tableName = tables.at(table);
    const std::string_view hullName = kHullNames[static_cast<std::size_t>(hull)];
    const auto arc = [&] {
        return random != nullptr && random->chance(kExtendedArcChance * static_cast<int>(hull)) ? kArcForwardExtended
                                                                                                 : kArcForward;
    };
    const auto special = [&](std::string_view list, double budget) -> double {
        const auto& rows = rowNames(p, list);
        for (std::size_t r = 0; r < rows.size(); ++r) {
            const int id = cell(p, list, r, "id");
            if (cell(p, list, r, "type") != 0 || d.hasSpecial(id) || !k.knowsTech(parts.specialTech(id))) {
                continue;
            }
            const double s = parts.specialSpace(k, id, hull);
            if (s <= budget) {
                d.specials |= std::uint64_t{1} << id;
                return budget - s;
            }
        }
        return budget;
    };
    double carry = 0;
    for (std::size_t c = 0; c < kColumns; ++c) {
        const auto percent = p.integer("ai_ship_design", {tableName, hullName}, kColumnNames[c]);
        if (percent < 0) {
            continue;
        }
        double budget = std::floor(space * static_cast<double>(percent) / 100.0) + carry;
        const auto fill = [&](const ShipParts::Weapon* w, int mods, int arcs, int ammo) {
            if (w == nullptr) {
                return;
            }
            WeaponMount m{w->number, 0, arcs, mods, ammo};
            const double s = parts.weaponSpace(k, m);
            if (s <= 0) {
                return;
            }
            m.count = static_cast<int>(std::floor(budget / s + 1e-9));
            if (m.count > 0) {
                budget -= m.count * s;
                d.weapons.push_back(m);
            }
        };
        switch (static_cast<Column>(c)) {
        case BeamSpecial:
            budget = special(kBeamSpecials, budget);
            break;
        case MissileSpecial:
            budget = special(kMissileSpecials, budget);
            break;
        case DefenseSpecial:
            budget = special(kDefenseSpecials, budget);
            break;
        case SpecialDevice:
            budget = special(kDeviceSpecials, budget);
            break;
        case Fighter:
            fill(bestWeapon(parts, k, {kFighter}), 0, kArcAll, 1);
            break;
        case Missiles: {
            const ShipParts::Weapon* w = bestWeapon(parts, k, {kMissile, kTorpedo});
            fill(w, 0, kArcAll, w != nullptr && w->weaponClass == kTorpedo ? w->ammo : shots);
            break;
        }
        case HeavyBeams:
            fill(bestWeapon(parts, k, {kBeam}, "hv"), kModHeavyMount, arc(), 0);
            break;
        case Beams:
            fill(bestWeapon(parts, k, {kBeam}), 0, arc(), 0);
            break;
        case PointDefense:
            fill(bestWeapon(parts, k, {kBeam}, "pd"), kModPointDefense, arc(), 0);
            break;
        case Bombs: {
            const ShipParts::Weapon* w = bestWeapon(parts, k, {kBomb});
            fill(w, 0, kArcAll, w != nullptr ? w->ammo : 0);
            break;
        }
        default:
            // The race's theme and the special weapons: still to come.
            break;
        }
        carry = budget;
    }
    return d;
}

std::vector<ShipDesign> startingDesigns(const Parameters& p, const ShipParts& parts, const Knowledge& k,
                                        Random& random) {
    std::vector<ShipDesign> designs;
    // Measured: the Scout, a frigate with a forward beam and extended fuel
    // tanks, shown with one of the eight frigate pictures.
    ShipDesign scout;
    scout.computer = parts.bestComputer(k);
    scout.drive = parts.bestDrive(k);
    scout.armor = parts.bestArmor(k);
    scout.shield = parts.bestShield(k);
    if (const ShipParts::Weapon* w = bestWeapon(parts, k, {kBeam})) {
        scout.weapons.push_back({w->number, 1, kArcForward, 0, 0});
    }
    if (k.knowsTech(parts.specialTech(kExtendedFuelTanks))) {
        scout.specials |= std::uint64_t{1} << kExtendedFuelTanks;
    }
    scout.image = static_cast<int>(random.below(8));
    parts.finish(scout, k);
    designs.push_back(std::move(scout));

    for (std::size_t h = 0; h + 1 < kDesignSlots; ++h) {
        const std::size_t t = random.pick(kStartTableWeights).value_or(0);
        ShipDesign d = autoDesign(p, parts, k, static_cast<Hull>(h), kStartTables[t], kStartTableShots[t], &random);
        d.nameIndex = static_cast<int>(8 * h + random.below(8));
        d.image = d.nameIndex;
        parts.finish(d, k);
        designs.push_back(std::move(d));
    }
    return designs;
}

} // namespace hegemon
