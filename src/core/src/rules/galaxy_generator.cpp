// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/galaxy_generator.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/random.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <stdexcept>
#include <string_view>

namespace hegemon {

int Star::satellites() const {
    return static_cast<int>(std::count_if(orbits.begin(), orbits.end(), [](int p) { return p != kNoPlanet; }));
}

StarNames StarNames::load(GameData& data) {
    const LbxArchive& lbx = data.archive("STARNAME.LBX");
    StarNames names;
    for (std::size_t entry = 0; entry < 2; ++entry) {
        const TextTable table = TextTable::parse(lbx.entry(entry));
        auto& out = entry == 0 ? names.home : names.other;
        for (std::size_t i = 0; i < table.size(); ++i) {
            out.push_back(table.at(i));
        }
    }
    if (names.home.size() < Races::kStockRaces) {
        throw FormatError("STARNAME.LBX: fewer home star names than races");
    }
    return names;
}

namespace {

// The names the fan patch's tables use for their rows and columns.
constexpr std::array<std::string_view, kStarColours> kColourNames = {"blue",   "white", "yellow",    "orange",
                                                                     "red",    "brown", "black_hole"};
constexpr std::array<std::string_view, 3> kAgeNames = {"young", "average", "old"};
constexpr std::array<std::string_view, kOrbits> kOrbitNames = {"orbit1", "orbit2", "orbit3", "orbit4", "orbit5"};
constexpr std::array<std::string_view, 10> kRollNames = {"random0", "random1", "random2", "random3", "random4",
                                                         "random5", "random6", "random7", "random8", "random9"};
constexpr std::array<std::string_view, 5> kSizeNames = {"tiny", "small", "medium", "large", "huge"};
constexpr std::array<std::string_view, 5> kMineralNames = {"ultra_poor", "poor", "abundant", "rich", "ultra_rich"};
constexpr std::array<std::string_view, kClimates> kClimateNames = {
    "toxic", "radiated", "barren", "desert", "tundra", "ocean", "swamp", "arid", "terran", "gaia"};
constexpr std::array<std::string_view, 4> kClimateGroupNames = {"bombarded", "hostile", "life_supporting",
                                                                "lifeless"};
constexpr std::array<std::string_view, 4> kMapSizeNames = {"small", "medium", "large", "huge"};

// Measured in the original (docs/rules/galaxy.md), not in the fan patch's
// tables: how many of a star's five orbits are taken, in percent, for 0 to
// 5 satellites, by star colour.
constexpr std::array<std::array<std::int64_t, kOrbits + 1>, kStarColours> kSatelliteCount = {{
    {11, 16, 9, 21, 21, 22},  // blue
    {9, 31, 21, 20, 19, 0},   // white
    {0, 10, 31, 20, 21, 18},  // yellow
    {0, 1, 27, 17, 24, 31},   // orange
    {7, 30, 34, 17, 12, 0},   // red
    {77, 23, 0, 0, 0, 0},     // brown
    {100, 0, 0, 0, 0, 0},     // black hole
}};
// How many satellites home systems have, in percent for 0 to 5, measured.
constexpr std::array<std::int64_t, kOrbits + 1> kHomeSatelliteCount = {0, 0, 0, 50, 34, 16};
// The colours of home stars, in percent, also measured.
constexpr std::array<std::int64_t, kStarColours> kHomeColour = {1, 15, 18, 22, 41, 3, 0};
// The orbit of Orion's planet, measured; Orion's other orbits hold an
// asteroid belt half of the time.
constexpr std::array<std::int64_t, kOrbits> kOrionOrbit = {0, 0, 8, 20, 12};
constexpr int kOrionBeltPercent = 50;
// Home systems have at least this many planets (the fan patch's /planets
// switch, whose default is 3).
constexpr int kDefaultHomePlanets = 3;

enum class Group { Bombarded, Hostile, LifeSupporting, Lifeless };

class Maker {
public:
    // Measured for the four map sizes of the fan patch's tables (small,
    // medium, large, huge): where stars may lie, how close two may come and
    // how far apart home stars are.
    struct Layout {
        int left, top, right, bottom; // the margins
        int apartX, apartY;           // no two stars closer in both
        double homesApart;
    };
    static constexpr std::array<Layout, 4> kLayout = {{
        {24, 23, 18, 26, 57, 50, 129.0},
        {31, 31, 38, 38, 60, 72, 290.0},
        {42, 42, 43, 47, 75, 71, 430.0},
        {63, 66, 54, 64, 90, 88, 651.0},
    }};
    // How often Orion is the nearest star to the middle of the home stars,
    // the second nearest and so on (measured).
    static constexpr std::array<std::int64_t, 10> kOrionRank = {5, 7, 7, 6, 5, 3, 3, 2, 1, 1};

    Maker(const Parameters& p, const StarNames& names, const GalaxySettings& s, Random& r)
        : p_(p), names_(names), s_(s), r_(r) {
        using Size = NewGameSettings::GalaxySize;
        switch (s.size) {
        case Size::Small:
            map_ = 0;
            break;
        case Size::Medium:
            map_ = 1;
            break;
        case Size::Large:
            map_ = 2;
            break;
        case Size::Cluster:
        case Size::Huge:
            map_ = 3;
            break;
        }
        age_ = static_cast<std::size_t>(s.age);
    }

    Galaxy make() {
        const std::string_view map = kMapSizeNames[map_];
        g_.width = static_cast<int>(p_.integer("map_generation", {map}, "width"));
        g_.height = static_cast<int>(p_.integer("map_generation", {map}, "height"));
        const auto count = static_cast<std::size_t>(p_.integer("stars_in_galaxy", {}, map));
        if (count < s_.players.size() + 1) {
            throw std::runtime_error("galaxy: fewer stars than players");
        }
        // When the home stars can't be as far apart as usual (many players
        // in a small galaxy), they may come closer, a tenth at a time.
        double apart = kLayout[map_].homesApart;
        for (int attempt = 0; attempt < 200; ++attempt) {
            if (attempt > 0 && attempt % 10 == 0) {
                apart *= 0.9;
            }
            g_.stars.clear();
            g_.planets.clear();
            g_.homes.clear();
            if (place(count) && chooseHomes(apart)) {
                chooseOrion();
                fill();
                name();
                return std::move(g_);
            }
        }
        throw std::runtime_error("galaxy: no room for the stars and home systems");
    }

private:
    std::int64_t cell(std::string_view table, std::string_view row, std::string_view column) const {
        return p_.integer(table, {row}, column);
    }

    // Stars at random where no other star is close in both directions, the
    // better of two places each time.
    bool place(std::size_t count) {
        const Layout& l = kLayout[map_];
        const int left = l.left, right = g_.width - l.right;
        const int top = l.top, bottom = g_.height - l.bottom;
        if (left > right || top > bottom) {
            return false;
        }
        const auto free = [&](int x, int y) {
            return std::all_of(g_.stars.begin(), g_.stars.end(), [&](const Star& s) {
                return std::abs(s.x - x) >= l.apartX || std::abs(s.y - y) >= l.apartY;
            });
        };
        const auto nearest = [&](int x, int y) {
            double best = 1e18;
            for (const auto& s : g_.stars) {
                best = std::min(best, std::hypot(s.x - x, s.y - y));
            }
            return best;
        };
        int tries = 0;
        while (g_.stars.size() < count) {
            int found = 0;
            int bx = 0, by = 0;
            double bd = -1;
            while (found < 2 && tries < 20000) {
                ++tries;
                const int x = r_.between(left, right), y = r_.between(top, bottom);
                if (!free(x, y)) {
                    continue;
                }
                ++found;
                const double d = nearest(x, y);
                if (d > bd) {
                    bd = d;
                    bx = x;
                    by = y;
                }
            }
            if (found == 0) {
                return false;
            }
            Star s;
            s.x = bx;
            s.y = by;
            g_.stars.push_back(s);
        }
        return true;
    }

    double distance(int a, int b) const {
        const Star& s = g_.stars[static_cast<std::size_t>(a)];
        const Star& t = g_.stars[static_cast<std::size_t>(b)];
        return std::hypot(s.x - t.x, s.y - t.y);
    }

    // Home stars at least `apart` from each other; the human's first.
    bool chooseHomes(double apart) {
        for (int attempt = 0; attempt < 50; ++attempt) {
            std::vector<int> chosen;
            std::vector<int> left;
            for (int i = 0; i < static_cast<int>(g_.stars.size()); ++i) {
                left.push_back(i);
            }
            while (chosen.size() < s_.players.size() && !left.empty()) {
                const auto at = r_.below(static_cast<std::uint32_t>(left.size()));
                const int star = left[at];
                left.erase(left.begin() + at);
                if (std::all_of(chosen.begin(), chosen.end(), [&](int other) { return distance(star, other) >= apart; })) {
                    chosen.push_back(star);
                }
            }
            if (chosen.size() == s_.players.size()) {
                for (std::size_t i = 0; i < chosen.size(); ++i) {
                    g_.homes.push_back({s_.players[i].race, chosen[i], kNoPlanet});
                }
                return true;
            }
        }
        return false;
    }

    // Orion: one of the stars nearest the middle of the home stars, then
    // moved to the front as star 0.
    void chooseOrion() {
        double cx = 0, cy = 0;
        for (const auto& h : g_.homes) {
            cx += g_.stars[static_cast<std::size_t>(h.star)].x;
            cy += g_.stars[static_cast<std::size_t>(h.star)].y;
        }
        cx /= static_cast<double>(g_.homes.size());
        cy /= static_cast<double>(g_.homes.size());
        std::vector<int> others;
        for (int i = 0; i < static_cast<int>(g_.stars.size()); ++i) {
            if (std::none_of(g_.homes.begin(), g_.homes.end(), [&](const HomeSystem& h) { return h.star == i; })) {
                others.push_back(i);
            }
        }
        std::sort(others.begin(), others.end(), [&](int a, int b) {
            const Star& s = g_.stars[static_cast<std::size_t>(a)];
            const Star& t = g_.stars[static_cast<std::size_t>(b)];
            return std::hypot(s.x - cx, s.y - cy) < std::hypot(t.x - cx, t.y - cy);
        });
        std::array<std::int64_t, kOrionRank.size()> w{};
        for (std::size_t i = 0; i < w.size() && i < others.size(); ++i) {
            w[i] = kOrionRank[i];
        }
        const int orion = others[r_.pick(w).value_or(0)];
        std::swap(g_.stars[0], g_.stars[static_cast<std::size_t>(orion)]);
        for (auto& h : g_.homes) {
            if (h.star == 0) {
                h.star = orion;
            }
        }
    }

    StarColour rollColour() {
        std::array<std::int64_t, kStarColours> w{};
        for (std::size_t c = 0; c < kStarColours; ++c) {
            w[c] = cell("star_class_chance", kColourNames[c], kAgeNames[age_]);
        }
        return static_cast<StarColour>(r_.pick(w).value_or(static_cast<std::size_t>(StarColour::Red)));
    }

    int roll10() { return static_cast<int>(r_.below(10)); }

    Planet rollSatellite(int star, int orbit) {
        const Star& s = g_.stars[static_cast<std::size_t>(star)];
        const auto colour = static_cast<std::size_t>(s.colour);
        Planet p;
        p.star = star;
        p.orbit = orbit;
        auto type = cell("orbit_to_satellite_type", kRollNames[static_cast<std::size_t>(roll10())],
                         kOrbitNames[static_cast<std::size_t>(orbit)]);
        if (type == 4) { // measured: an asteroid belt or a planet, even odds
            type = r_.below(2) == 0 ? 1 : 3;
        }
        p.type = type == 1 ? SatelliteType::AsteroidBelt : type == 2 ? SatelliteType::GasGiant : SatelliteType::Planet;
        p.minerals = static_cast<Minerals>(
            std::clamp<std::int64_t>(cell("class_to_mineral", kRollNames[static_cast<std::size_t>(roll10())],
                                          kColourNames[std::min<std::size_t>(colour, 5)]),
                                     0, 4));
        if (p.type != SatelliteType::Planet) {
            return p;
        }
        // planet_size_chance counts up to 10: a roll of 1 to 10 takes the
        // first size it doesn't pass.
        const int roll = roll10() + 1;
        p.size = PlanetSize::Huge;
        for (std::size_t i = 0; i < kSizeNames.size(); ++i) {
            if (roll <= p_.integer("planet_size_chance", {}, kSizeNames[i])) {
                p.size = static_cast<PlanetSize>(i);
                break;
            }
        }
        p.gravity = gravity(p.minerals, p.size);
        const auto group = static_cast<std::size_t>(std::clamp<std::int64_t>(
            cell("orbit_to_climate_group", kColourNames[std::min<std::size_t>(colour, 5)],
                 kOrbitNames[static_cast<std::size_t>(orbit)]),
            0, 3));
        p.climate = rollClimate(group);
        return p;
    }

    Gravity gravity(Minerals m, PlanetSize size) const {
        return static_cast<Gravity>(std::clamp<std::int64_t>(
            p_.integer("planet_size_to_gravity", {kMineralNames[static_cast<std::size_t>(m)]},
                       kSizeNames[static_cast<std::size_t>(size)]),
            0, 2));
    }

    Climate rollClimate(std::size_t group) {
        const std::string_view table = age_ == 2 ? "climate_chance_old_gal" : "climate_chance_young_avg_gal";
        std::array<std::int64_t, kClimates> w{};
        for (std::size_t c = 0; c < kClimates; ++c) {
            w[c] = cell(table, kClimateNames[c], kClimateGroupNames[group]);
        }
        return static_cast<Climate>(r_.pick(w).value_or(static_cast<std::size_t>(Climate::Barren)));
    }

    // The satellites of an ordinary star.
    void rollSystem(int star, std::span<const std::int64_t> counts) {
        Star& s = g_.stars[static_cast<std::size_t>(star)];
        const auto n = r_.pick(counts).value_or(0);
        for (std::size_t k = 0; k < n; ++k) {
            std::array<std::int64_t, kOrbits> w{};
            for (std::size_t o = 0; o < kOrbits; ++o) {
                w[o] = s.orbits[o] == kNoPlanet ? cell("satellite_orbit_chance", kOrbitNames[o], kAgeNames[age_]) : 0;
            }
            const auto orbit = r_.pick(w);
            if (!orbit) {
                break;
            }
            s.orbits[*orbit] = add(rollSatellite(star, static_cast<int>(*orbit)));
        }
    }

    int add(const Planet& p) {
        g_.planets.push_back(p);
        return static_cast<int>(g_.planets.size() - 1);
    }

    void clearSystem(int star) {
        Star& s = g_.stars[static_cast<std::size_t>(star)];
        // Planets of the system being made again are always the last ones.
        int lowest = static_cast<int>(g_.planets.size());
        for (auto& o : s.orbits) {
            if (o != kNoPlanet) {
                lowest = std::min(lowest, o);
                o = kNoPlanet;
            }
        }
        g_.planets.resize(static_cast<std::size_t>(lowest));
    }

    void fill() {
        // Orion.
        Star& orion = g_.stars[0];
        orion.colour = static_cast<StarColour>(r_.below(5)); // blue to red
        orion.special = Special::Orion;
        orion.name = "Orion";
        const auto at = r_.pick(kOrionOrbit).value_or(3);
        for (std::size_t o = 0; o < kOrbits; ++o) {
            if (o == at) {
                Planet p;
                p.star = 0;
                p.orbit = static_cast<int>(o);
                p.size = PlanetSize::Huge;
                p.minerals = Minerals::UltraRich;
                p.gravity = Gravity::Normal;
                p.climate = Climate::Gaia;
                p.special = Special::Orion;
                orion.orbits[o] = add(p);
            } else if (r_.chance(kOrionBeltPercent)) {
                Planet p = rollSatellite(0, static_cast<int>(o));
                p.type = SatelliteType::AsteroidBelt;
                orion.orbits[o] = add(p);
            }
        }
        // The others.
        for (int i = 1; i < static_cast<int>(g_.stars.size()); ++i) {
            const bool home = std::any_of(g_.homes.begin(), g_.homes.end(), [&](const HomeSystem& h) {
                return h.star == i;
            });
            Star& s = g_.stars[static_cast<std::size_t>(i)];
            s.colour = home ? static_cast<StarColour>(r_.pick(kHomeColour).value_or(4)) : rollColour();
            if (home) {
                makeHome(i);
            } else {
                rollSystem(i, kSatelliteCount[static_cast<std::size_t>(s.colour)]);
            }
        }
    }

    void makeHome(int star) {
        auto home = std::find_if(g_.homes.begin(), g_.homes.end(), [&](const HomeSystem& h) { return h.star == star; });
        const RacePicks& picks = s_.players[static_cast<std::size_t>(home - g_.homes.begin())].picks;
        Star& s = g_.stars[static_cast<std::size_t>(star)];
        std::vector<int> planets;
        for (int attempt = 0; attempt < 1000; ++attempt) {
            clearSystem(star);
            rollSystem(star, kHomeSatelliteCount);
            planets.clear();
            for (const int o : s.orbits) {
                if (o != kNoPlanet && g_.planets[static_cast<std::size_t>(o)].type == SatelliteType::Planet) {
                    planets.push_back(o);
                }
            }
            if (static_cast<int>(planets.size()) >= kDefaultHomePlanets) {
                break;
            }
        }
        if (planets.empty()) { // can't happen with the stock tables
            throw std::runtime_error("galaxy: a home system without planets");
        }
        const int world = planets[r_.below(static_cast<std::uint32_t>(planets.size()))];
        home->planet = world;
        Planet& p = g_.planets[static_cast<std::size_t>(world)];
        p.size = picks[RacePicks::LargeHomeWorld] != 0 ? PlanetSize::Large : PlanetSize::Medium;
        p.minerals = picks[RacePicks::HomeWorldRichness] == 1   ? Minerals::Rich
                     : picks[RacePicks::HomeWorldRichness] == 2 ? Minerals::Poor
                                                                : Minerals::Abundant;
        p.gravity = picks[RacePicks::LowGravity] != 0    ? Gravity::Low
                    : picks[RacePicks::HighGravity] != 0 ? Gravity::Heavy
                                                         : Gravity::Normal;
        p.climate = picks[RacePicks::Aquatic] != 0 ? Climate::Ocean : Climate::Terran;
        p.special = Special::None;
        // Measured: no other planet of a home system is Terran.
        for (const int o : planets) {
            Planet& q = g_.planets[static_cast<std::size_t>(o)];
            if (o != world && q.climate == Climate::Terran) {
                const auto colour = std::min<std::size_t>(static_cast<std::size_t>(s.colour), 5);
                const auto group = static_cast<std::size_t>(std::clamp<std::int64_t>(
                    cell("orbit_to_climate_group", kColourNames[colour], kOrbitNames[static_cast<std::size_t>(q.orbit)]),
                    0, 3));
                for (int k = 0; k < 100 && q.climate == Climate::Terran; ++k) {
                    q.climate = rollClimate(group);
                }
            }
        }
    }

    // The names: the list mixed by as many swaps of two names as it has,
    // then star i takes name i; home stars take their race's name.
    void name() {
        std::vector<std::string> list = names_.other;
        const auto n = static_cast<std::uint32_t>(list.size());
        for (std::uint32_t k = 0; k < n; ++k) {
            std::swap(list[r_.below(n)], list[r_.below(n)]);
        }
        for (std::size_t i = 1; i < g_.stars.size(); ++i) {
            g_.stars[i].name = i < list.size() ? list[i] : "Star " + std::to_string(i);
        }
        for (const auto& h : g_.homes) {
            const auto race = static_cast<std::size_t>(h.race);
            if (race < names_.home.size()) {
                g_.stars[static_cast<std::size_t>(h.star)].name = names_.home[race];
            }
        }
    }

    const Parameters& p_;
    const StarNames& names_;
    const GalaxySettings& s_;
    Random& r_;
    std::size_t map_ = 1;
    std::size_t age_ = 1;
    Galaxy g_;
};

} // namespace

GalaxyGenerator::GalaxyGenerator(const Parameters& parameters, StarNames names)
    : p_(parameters), names_(std::move(names)) {}

Galaxy GalaxyGenerator::generate(const GalaxySettings& settings, Random& random) const {
    return Maker(p_, names_, settings, random).make();
}

} // namespace hegemon
