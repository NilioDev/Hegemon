// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/combat.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/random.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
#include <string_view>

namespace hegemon {

namespace {

// The fan patch manual's tables. To hit, by range (three squares a range):
// the penalty, which stays at its last value further off.
constexpr std::array<int, 9> kHitPenalty = {0, 0, 10, 20, 30, 40, 55, 70, 85};
// What a dissipating beam keeps of its damage, in percent, by range.
constexpr std::array<int, 9> kDissipation = {100, 100, 90, 80, 70, 60, 50, 40, 35};
constexpr int kHitThreshold = 40, kMaxThreshold = 95, kLuckyRoll = 95, kSureHit = 99;
// Heavy mounts do half again as much, point defence half (additive with
// the dissipation, as the manual says).
constexpr int kHeavyBonus = 50, kPointDefenseLoss = 50;
// Weapon classes.
constexpr int kBeam = 0;

// Monster names in the fan patch's tables, by MonsterKind.
struct MonsterRow {
    const char* design;     // its design table, nullptr for the Guardian
    const char* common;     // the table of what system and event monsters share
    const char* hitPoints;  // its column of monster_hit_points
    const char* drive;      // its column of monster_drive
    const char* name;
};
constexpr std::array<MonsterRow, 6> kMonsters = {{
    {nullptr, nullptr, nullptr, "crystal_and_guardian", "Guardian"},
    {"system_amoeba", nullptr, "system_amoeba", "amoeba_and_hydra", "Amoeba"},
    {"system_crystal", "system_and_event_crystal", "system_crystal_hydra_dragon", "crystal_and_guardian", "Crystal"},
    {"system_dragon", "system_and_event_dragon", "system_crystal_hydra_dragon", "dragon_and_eel", "Dragon"},
    {"system_eel", "system_and_event_eel", "system_eel", "dragon_and_eel", "Eel"},
    {"system_hydra", "system_and_event_hydra", "system_crystal_hydra_dragon", "amoeba_and_hydra", "Hydra"},
}};
// The Guardian has no table in the fan patch's settings (assumed values,
// see docs/rules/combat.md).
constexpr int kGuardianStructure = 10000, kGuardianComputer = 5;

constexpr std::array<const char*, kHullSizes> kHullNames = {"frigate", "destroyer", "cruiser",
                                                            "battleship", "titan", "doomstar"};
constexpr std::array<const char*, kHullSizes> kSpeedColumns = {"ff_min", "dd_min", "ca_min",
                                                               "bb_min", "tt_min", "ds_min"};

std::size_t columnOf(const Parameters& p, std::string_view name, std::string_view column) {
    const ParameterDefinition* d = p.definition(name);
    for (std::size_t i = 0; d != nullptr && i < d->columns.size(); ++i) {
        if (d->columns[i] == column) {
            return i;
        }
    }
    throw std::out_of_range("setting " + std::string(name) + " has no column " + std::string(column));
}

int cell(const Parameters& p, std::string_view name, std::size_t row, std::string_view column) {
    return static_cast<int>(p.integer(name, row, columnOf(p, name, column)));
}

// The row of the weapon table with `number`, or -1.
std::ptrdiff_t weaponRow(const Parameters& p, int number) {
    const ParameterDefinition* d = p.definition("weapon");
    const std::size_t rows = d == nullptr ? 0 : d->rowCount();
    for (std::size_t r = 0; r < rows; ++r) {
        if (cell(p, "weapon", r, "number") == number) {
            return static_cast<std::ptrdiff_t>(r);
        }
    }
    return -1;
}

bool hasFlag(const std::string& flags, std::string_view flag) {
    std::size_t at = 0;
    while (at <= flags.size()) {
        const std::size_t end = std::min(flags.find(',', at), flags.size());
        if (std::string_view(flags).substr(at, end - at) == flag) {
            return true;
        }
        at = end + 1;
    }
    return false;
}

std::optional<CombatWeapon> combatWeapon(const Parameters& p, int number, int count, int mods, int ammo) {
    const std::ptrdiff_t row = weaponRow(p, number);
    if (row < 0 || count <= 0) {
        return std::nullopt;
    }
    const auto r = static_cast<std::size_t>(row);
    CombatWeapon w;
    w.weapon = number;
    w.weaponClass = cell(p, "weapon", r, "class");
    w.count = count;
    w.mods = mods;
    const std::string natural = p.value("weapon", r, columnOf(p, "weapon", "natural_mods")).text;
    if (hasFlag(natural, "dr")) {
        w.natural |= kNaturalDoubleRange;
    }
    if (hasFlag(natural, "nr")) {
        w.natural |= kNaturalNoDissipation;
    }
    // The fan patch fights with the strategic column (its manual).
    w.minDamage = cell(p, "weapon", r, "strategic_minimum_damage");
    w.maxDamage = cell(p, "weapon", r, "strategic_maximum_damage");
    w.ammo = w.weaponClass == kBeam ? -1 : ammo;
    return w;
}

int hullCell(const Parameters& p, Hull hull, std::string_view column) {
    return static_cast<int>(p.integer("hull", {kHullNames[static_cast<std::size_t>(hull)]}, column));
}

int squaresBetween(const Combatant& a, const Combatant& b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

int rangeOf(int squares) {
    return (squares + kSquaresARange - 1) / kSquaresARange;
}

// The longest reach of a combatant's beams, in squares: as far as the
// to-hit table goes.
constexpr int kBeamReach = kSquaresARange * (static_cast<int>(kHitPenalty.size()) - 1);

} // namespace

int rangePenalty(int squares, int mods, int natural) {
    int range = rangeOf(squares);
    if ((mods & kModHeavyMount) != 0) {
        range /= 2;
    } else if ((mods & kModPointDefense) != 0) {
        range *= 2;
    }
    range = std::min(range, static_cast<int>(kHitPenalty.size()) - 1);
    int penalty = kHitPenalty[static_cast<std::size_t>(range)];
    if ((natural & kNaturalDoubleRange) != 0) {
        penalty *= 2;
    }
    return penalty;
}

BeamShot beamShot(const CombatWeapon& weapon, int attack, int squares, int roll) {
    // The damage it can do at this range.
    int percent = 100;
    if ((weapon.natural & kNaturalNoDissipation) == 0) {
        const auto range = std::min(static_cast<std::size_t>(rangeOf(squares)), kDissipation.size() - 1);
        percent = kDissipation[range];
    }
    if ((weapon.mods & kModHeavyMount) != 0) {
        percent += kHeavyBonus;
    } else if ((weapon.mods & kModPointDefense) != 0) {
        percent -= kPointDefenseLoss;
    }
    const auto scaled = [&](int damage) { return std::max(1, (damage * percent + 50) / 100); };
    const int low = scaled(weapon.minDamage), high = scaled(weapon.maxDamage);

    BeamShot shot;
    if (roll > kLuckyRoll || attack >= kSureHit) {
        shot.hit = true;
        shot.damage = high;
        return shot;
    }
    const int threshold = std::min(kHitThreshold + rangePenalty(squares, weapon.mods, weapon.natural), kMaxThreshold);
    const int rolled = std::min(roll + attack, 100);
    if (rolled < threshold) {
        return shot;
    }
    shot.hit = true;
    shot.damage = std::min(high, low + (high - low + 1) * (rolled - threshold) / (100 - threshold));
    return shot;
}

void takeHit(Combatant& target, int damage) {
    damage = std::max(0, damage - target.shield);
    const int held = std::min(damage, target.armor);
    target.armor -= held;
    damage -= held;
    target.structure -= damage;
    if (target.structure <= 0) {
        target.structure = 0;
        target.destroyed = true;
    }
}

Battle monsterBattle(const Game& game, const Parameters& p, int monster) {
    const Monster& m = game.galaxy.monsters.at(static_cast<std::size_t>(monster));
    const MonsterRow& row = kMonsters.at(static_cast<std::size_t>(m.kind));
    Battle battle;
    battle.star = m.star;

    Combatant beast;
    beast.kind = Combatant::Kind::Monster;
    beast.monster = monster;
    beast.name = row.name;
    int computer = kGuardianComputer;
    if (row.design != nullptr) {
        beast.hull = static_cast<Hull>(std::clamp(cell(p, row.design, 0, "hull_size"), 0, static_cast<int>(kHullSizes) - 1));
        beast.maxStructure = static_cast<int>(p.integer("monster_hit_points", {}, row.hitPoints));
        const ParameterDefinition* d = p.definition(row.design);
        const ParameterDefinition* common = row.common != nullptr ? p.definition(row.common) : nullptr;
        const auto has = [](const ParameterDefinition* def, std::string_view column) {
            return def != nullptr && std::find(def->columns.begin(), def->columns.end(), column) != def->columns.end();
        };
        if (has(d, "computer")) {
            computer = cell(p, row.design, 0, "computer");
        } else if (has(common, "computer")) {
            computer = cell(p, row.common, 0, "computer");
        } else {
            computer = 0;
        }
        for (int n = 1; has(d, "weapon_" + std::to_string(n) + "_type"); ++n) {
            const std::string prefix = "weapon_" + std::to_string(n) + "_";
            if (auto w = combatWeapon(p, cell(p, row.design, 0, prefix + "type"), cell(p, row.design, 0, prefix + "amount"),
                                      0, cell(p, row.design, 0, prefix + "ammo"))) {
                beast.weapons.push_back(*w);
            }
        }
    } else {
        beast.hull = Hull::Titan;
        beast.maxStructure = kGuardianStructure;
    }
    beast.structure = beast.maxStructure;
    if (computer > 0) {
        beast.beamAttack = cell(p, "computer", static_cast<std::size_t>(computer - 1), "bonus");
    }
    const int driveRow = static_cast<int>(p.integer("monster_drive", {}, row.drive)) - 1;
    if (driveRow >= 0) {
        beast.speed = cell(p, "drive", static_cast<std::size_t>(driveRow), kSpeedColumns[static_cast<std::size_t>(beast.hull)]);
    }
    beast.beamDefense = 5 * beast.speed;
    beast.armed = !beast.weapons.empty();
    // Seen: the monster starts on the right, the ships on the left.
    beast.x = kFieldWidth * 3 / 4;
    beast.y = kFieldHeight / 2;
    battle.units.push_back(std::move(beast));

    int placed = 0;
    for (std::size_t i = 0; i < game.ships.size(); ++i) {
        const Ship& s = game.ships[i];
        if (s.star != m.star || s.player < 0 || static_cast<std::size_t>(s.player) >= game.players.size()) {
            continue;
        }
        const Player& owner = game.players[static_cast<std::size_t>(s.player)];
        Combatant c;
        c.owner = s.player;
        c.ship = static_cast<int>(i);
        const ShipDesign* design = nullptr;
        if (s.kind == ShipKind::Scout && !owner.designs.empty()) {
            design = &owner.designs.front();
        } else if (s.kind == ShipKind::Combat && s.design >= 0 && static_cast<std::size_t>(s.design) < owner.designs.size()) {
            design = &owner.designs[static_cast<std::size_t>(s.design)];
        }
        if (design != nullptr) {
            c.name = design->name;
            c.hull = design->hull;
            const int crew = crewLevel(p, s.experience);
            c.beamAttack = beamAttack(p, owner.picks, *design, crew);
            c.beamDefense = beamDefense(p, owner.picks, *design, crew);
            c.speed = design->combatSpeed;
            for (const WeaponMount& mount : design->weapons) {
                if (auto w = combatWeapon(p, mount.weapon, mount.count, mount.mods, mount.ammo)) {
                    c.weapons.push_back(*w);
                }
            }
            if (design->shield > 0) {
                c.shield = cell(p, "shield", static_cast<std::size_t>(design->shield - 1), "shield_class");
            }
            const int armorBonus = design->armor > 0 ? cell(p, "armor", static_cast<std::size_t>(design->armor - 1), "ships_bonus") : 0;
            c.maxArmor = hullCell(p, c.hull, "armor_hp") * (100 + armorBonus) / 100;
        }
        c.maxStructure = hullCell(p, c.hull, "structure_hp");
        c.structure = c.maxStructure;
        c.armor = c.maxArmor;
        c.armed = !c.weapons.empty();
        // Seen: the ships stand side by side (two squares apart assumed),
        // a quarter of the way in.
        c.x = kFieldWidth / 4;
        c.y = kFieldHeight / 2 - 2 + 2 * placed;
        ++placed;
        battle.units.push_back(std::move(c));
    }
    return battle;
}

void fightAuto(Battle& battle, const Parameters& p, Random& random) {
    const int turns = p.definition("max_combat_turns") != nullptr ? static_cast<int>(p.integer("max_combat_turns")) : 50;
    const auto side = [](const Combatant& c) { return c.kind == Combatant::Kind::Monster ? -1 : c.owner; };
    const auto armedAlive = [&](int s) {
        return std::any_of(battle.units.begin(), battle.units.end(),
                           [&](const Combatant& c) { return !c.destroyed && c.armed && side(c) == s; });
    };
    const auto sides = [&] {
        std::vector<int> s;
        for (const Combatant& c : battle.units) {
            if (!c.destroyed && std::find(s.begin(), s.end(), side(c)) == s.end()) {
                s.push_back(side(c));
            }
        }
        return s;
    };

    // The manual: a fleet with no warship is destroyed out of hand.
    for (int s : sides()) {
        if (!armedAlive(s)) {
            for (int other : sides()) {
                if (other != s && armedAlive(other)) {
                    for (Combatant& c : battle.units) {
                        if (side(c) == s) {
                            c.destroyed = true;
                        }
                    }
                    break;
                }
            }
        }
    }

    // Faster units act first (assumed), the list's order among equals.
    std::vector<std::size_t> order(battle.units.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return battle.units[a].speed > battle.units[b].speed; });

    for (battle.rounds = 0; battle.rounds < turns && sides().size() > 1; ++battle.rounds) {
        for (std::size_t i : order) {
            Combatant& self = battle.units[i];
            if (self.destroyed || !self.armed) {
                continue;
            }
            // The nearest enemy.
            Combatant* target = nullptr;
            for (Combatant& c : battle.units) {
                if (!c.destroyed && side(c) != side(self) &&
                    (target == nullptr || squaresBetween(self, c) < squaresBetween(self, *target))) {
                    target = &c;
                }
            }
            if (target == nullptr) {
                break;
            }
            // Close in to one square (assumed), a square a movement point.
            for (int step = 0; step < self.speed && squaresBetween(self, *target) > 1; ++step) {
                self.x += (target->x > self.x) - (target->x < self.x);
                self.y += (target->y > self.y) - (target->y < self.y);
            }
            const int squares = squaresBetween(self, *target);
            for (CombatWeapon& w : self.weapons) {
                if (w.weaponClass != kBeam || squares > kBeamReach) {
                    continue; // missiles and the rest come later
                }
                for (int n = 0; n < w.count && !target->destroyed; ++n) {
                    const int roll = static_cast<int>(random.below(100)) + 1;
                    const BeamShot shot = beamShot(w, self.beamAttack - target->beamDefense, squares, roll);
                    if (shot.hit) {
                        takeHit(*target, shot.damage);
                    }
                }
            }
        }
    }
    const auto left = sides();
    if (left.size() == 1) {
        battle.winner = left.front();
    }
}

std::vector<BattleReport> fightMonsters(Game& game, const Parameters& parameters, Random& random) {
    std::vector<BattleReport> reports;
    std::vector<bool> gone(game.ships.size(), false);
    for (std::size_t m = 0; m < game.galaxy.monsters.size(); ++m) {
        const Monster& monster = game.galaxy.monsters[m];
        Battle battle = monsterBattle(game, parameters, static_cast<int>(m));
        if (battle.units.size() < 2) {
            continue;
        }
        fightAuto(battle, parameters, random);
        std::vector<int> players;
        for (const Combatant& c : battle.units) {
            if (c.kind == Combatant::Kind::Ship && std::find(players.begin(), players.end(), c.owner) == players.end()) {
                players.push_back(c.owner);
            }
        }
        for (int player : players) {
            BattleReport r;
            r.star = monster.star;
            r.monster = monster.kind;
            r.player = player;
            for (const Combatant& c : battle.units) {
                if (c.kind == Combatant::Kind::Ship && c.owner == player && c.destroyed) {
                    ++r.shipsLost;
                    gone[static_cast<std::size_t>(c.ship)] = true;
                }
            }
            reports.push_back(r);
        }
    }
    std::vector<Ship> kept;
    for (std::size_t i = 0; i < game.ships.size(); ++i) {
        if (!gone[i]) {
            kept.push_back(game.ships[i]);
        }
    }
    game.ships = std::move(kept);
    std::stable_sort(reports.begin(), reports.end(), [&](const BattleReport& a, const BattleReport& b) {
        const bool ha = game.players[static_cast<std::size_t>(a.player)].human;
        const bool hb = game.players[static_cast<std::size_t>(b.player)].human;
        return ha && !hb;
    });
    return reports;
}

} // namespace hegemon
