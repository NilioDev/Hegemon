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
constexpr std::array<std::string_view, 11> kSpecialNames = {
    "no_special", "wormhole", "space_debris", "pirate_cache", "gold_deposits", "gem_deposits",
    "natives",    "splinter_colony", "hero", "monster", "ancient_artifacts"};
constexpr std::array<std::string_view, 4> kMapSizeNames = {"small", "medium", "large", "huge"};

// How many satellites home systems have, in percent for 0 to 5, measured.
constexpr std::array<std::int64_t, kOrbits + 1> kHomeSatelliteCount = {0, 0, 0, 50, 34, 16};
// The colours of home stars, in percent, also measured.
constexpr std::array<std::int64_t, kStarColours> kHomeColour = {1, 15, 18, 22, 41, 3, 0};
// The orbit of Orion's planet, measured; Orion's other orbits hold an
// asteroid belt half of the time.
constexpr std::array<std::int64_t, kOrbits> kOrionOrbit = {0, 0, 8, 20, 12};
constexpr int kOrionBeltPercent = 50;
// STARBG.LBX: twelve nebulae, each at four scales, the galaxy map's first.
constexpr std::size_t kFirstNebula = 6;
constexpr std::size_t kNebulaPictures = 12;
// A star is in a nebula where its picture's colour index is above this.
constexpr std::uint8_t kNebulaLight = 5;
// How many in a hundred of the table's black holes stay black holes
// (measured).
constexpr std::int64_t kBlackHoleKept = 60;
// The chance that a system with a special find is guarded (measured).
constexpr int kGuardPercent = 24;
// Which monster guards a system, by its special find (measured counts for
// the amoeba, crystal, dragon, eel and hydra).
constexpr std::array<std::array<std::int64_t, 5>, 11> kGuardKind = {{
    {0, 0, 0, 0, 0},  // none
    {0, 0, 0, 0, 0},  // wormhole
    {2, 1, 0, 0, 5},  // space debris
    {4, 0, 4, 0, 5},  // pirate cache
    {3, 2, 0, 17, 5}, // gold deposits
    {4, 0, 5, 9, 3},  // gem deposits
    {3, 6, 19, 0, 13}, // natives
    {6, 5, 0, 7, 4},  // splinter colony
    {7, 1, 5, 0, 4},  // hero
    {0, 0, 0, 0, 0},  // monster
    {3, 7, 5, 0, 9},  // ancient artifacts
}};
// The climates natives, splinter colonies and ancient artifacts have, in
// percent from desert to gaia (measured).
constexpr std::array<std::int64_t, 7> kLivelyClimate = {16, 18, 7, 12, 8, 25, 13};
// Home systems have at least this many planets (the fan patch's /planets
// switch, whose default is 3).
constexpr int kDefaultHomePlanets = 3;
// Map units in a parsec: warp_interdictor_range says 8100 is 3 parsecs
// squared, so 90 units are 3 parsecs.
constexpr double kParsec = 30.0;
// Every home system of a race that eats has a planet that grows food
// within this many parsecs (the fan patch's manual, and measured).
constexpr double kFoodWorldParsecs = 6.0;

enum class Group { Bombarded, Hostile, LifeSupporting, Lifeless };

class Maker {
public:
    // Measured for the four map sizes of the fan patch's tables (small,
    // medium, large, huge) and for a cluster galaxy: where stars may lie, how close two may come and
    // how far apart home stars and the ends of a wormhole are and how many
    // systems monsters guard.
    struct Layout {
        int left, top, right, bottom; // the margins
        int apartX, apartY;           // no two stars closer in both
        double homesApart;
        double wormholeApart; // the ends of a wormhole at least
        int monsters;         // guarded systems at most
        std::array<std::int64_t, 5> nebulae; // percent for 0 to 4 nebulae
        int nebulaApart;      // corners at least this far across or down
        int nebulaW, nebulaH; // the area nebulae lie in (0: the map's)
    };
    static constexpr std::array<Layout, 5> kLayout = {{
        {24, 23, 18, 26, 57, 50, 129.0, 125.0, 1, {65, 35, 0, 0, 0}, 270, 0, 0},
        {31, 31, 38, 38, 60, 74, 290.0, 120.0, 3, {0, 23, 77, 0, 0}, 270, 0, 0},
        {42, 42, 43, 47, 75, 71, 430.0, 150.0, 5, {0, 55, 10, 35, 0}, 270, 0, 0},
        {63, 66, 54, 64, 90, 88, 651.0, 210.0, 7, {0, 0, 33, 40, 27}, 270, 0, 0},
        {31, 33, 28, 39, 60, 74, 430.0, 190.0, 7, {0, 0, 33, 27, 40}, 218, 759, 600}, // cluster
    }};
    // How often Orion is the nearest star to the middle of the home stars,
    // the second nearest and so on (measured).
    static constexpr std::array<std::int64_t, 10> kOrionRank = {5, 7, 7, 6, 5, 3, 3, 2, 1, 1};

    Maker(const Parameters& p, const StarNames& names, const std::vector<Canvas>& nebulae, const GalaxySettings& s,
          Random& r)
        : p_(p), names_(names), nebulaPictures_(nebulae), s_(s), r_(r) {
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
        case Size::Huge:
            map_ = 3;
            break;
        case Size::Cluster: // a large map with as many stars as a huge one
            map_ = 2;
            stars_ = 3;
            layout_ = 4;
            break;
        }
        if (s.size != Size::Cluster) {
            stars_ = map_;
            layout_ = map_;
        }
        age_ = static_cast<std::size_t>(s.age);
    }

    Galaxy make() {
        const std::string_view map = kMapSizeNames[map_];
        g_.width = static_cast<int>(p_.integer("map_generation", {map}, "width"));
        g_.height = static_cast<int>(p_.integer("map_generation", {map}, "height"));
        g_.cluster = s_.size == NewGameSettings::GalaxySize::Cluster;
        const auto count = static_cast<std::size_t>(p_.integer("stars_in_galaxy", {}, kMapSizeNames[stars_]));
        if (count < s_.players.size() + 1) {
            throw std::runtime_error("galaxy: fewer stars than players");
        }
        // When the home stars can't be as far apart as usual (many players
        // in a small galaxy), they may come closer, a tenth at a time.
        double apart = kLayout[layout_].homesApart;
        for (int attempt = 0; attempt < 200; ++attempt) {
            if (attempt > 0 && attempt % 10 == 0) {
                apart *= 0.9;
            }
            g_.stars.clear();
            g_.planets.clear();
            g_.homes.clear();
            g_.monsters.clear();
            g_.nebulae.clear();
            if (place(count) && chooseHomes(apart)) {
                chooseOrion();
                placeNebulae();
                fill();
                specials();
                monsters();
                if (foodWorlds()) {
                    name();
                    return std::move(g_);
                }
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
        const Layout& l = kLayout[layout_];
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

    // Nebulae: how many by the galaxy's size, pictures at random, apart
    // from each other (measured). Then which stars lie in one.
    void placeNebulae() {
        g_.nebulae.clear();
        if (nebulaPictures_.empty()) {
            return;
        }
        const Layout& l = kLayout[layout_];
        const int areaW = l.nebulaW > 0 ? l.nebulaW : g_.width;
        const int areaH = l.nebulaH > 0 ? l.nebulaH : g_.height;
        const auto count = r_.pick(l.nebulae).value_or(0);
        for (std::size_t n = 0; n < count; ++n) {
            for (int tries = 0; tries < 200; ++tries) {
                Nebula b;
                b.picture = static_cast<int>(r_.below(static_cast<std::uint32_t>(nebulaPictures_.size())));
                const Canvas& c = nebulaPictures_[static_cast<std::size_t>(b.picture)];
                if (c.width() > areaW || c.height() > areaH) {
                    break;
                }
                b.x = r_.between(0, areaW - c.width());
                b.y = r_.between(0, areaH - c.height());
                if (std::all_of(g_.nebulae.begin(), g_.nebulae.end(), [&](const Nebula& o) {
                        return std::abs(o.x - b.x) >= l.nebulaApart || std::abs(o.y - b.y) >= l.nebulaApart;
                    })) {
                    g_.nebulae.push_back(b);
                    break;
                }
            }
        }
        for (auto& s : g_.stars) {
            s.inNebula = std::any_of(g_.nebulae.begin(), g_.nebulae.end(), [&](const Nebula& b) {
                const Canvas& c = nebulaPictures_[static_cast<std::size_t>(b.picture)];
                const int px = s.x - b.x, py = s.y - b.y;
                return px >= 0 && py >= 0 && px < c.width() && py < c.height() && c.at(px, py) > kNebulaLight;
            });
        }
    }

    // A star's colour; no black holes in nebulae (fan patch 1.51 rolls
    // such a star again from the table).
    StarColour rollColour(bool inNebula, bool nearHome) {
        std::array<std::int64_t, kStarColours> w{};
        for (std::size_t c = 0; c < kStarColours; ++c) {
            w[c] = cell("star_class_chance", kColourNames[c], kAgeNames[age_]);
        }
        if (inNebula) {
            w[static_cast<std::size_t>(StarColour::BlackHole)] = 0;
        }
        // Measured: black holes are rarer than the table says (about 2.4 in
        // 100 stars, not 4, away from home stars too), and none lies within
        // 100 of a home star.
        if (nearHome) {
            w[static_cast<std::size_t>(StarColour::BlackHole)] = 0;
        } else {
            w[static_cast<std::size_t>(StarColour::BlackHole)] =
                w[static_cast<std::size_t>(StarColour::BlackHole)] * kBlackHoleKept / 100;
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

    // A satellite rolled for this orbit until it is a planet.
    Planet rollPlanet(int star, int orbit) {
        Planet p = rollSatellite(star, orbit);
        for (int k = 0; k < 100 && p.type != SatelliteType::Planet; ++k) {
            p = rollSatellite(star, orbit);
        }
        if (p.type != SatelliteType::Planet) { // the orbit never has planets
            p.type = SatelliteType::Planet;
            p.size = PlanetSize::Medium;
            p.gravity = gravity(p.minerals, p.size);
        }
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

    // How likely 0 to 5 satellites are for a star of this colour:
    // class_to_num_satellites, a roll from 0 to 9 the row and the colour
    // the column. Black holes have none.
    std::array<std::int64_t, kOrbits + 1> satelliteCounts(StarColour colour) const {
        std::array<std::int64_t, kOrbits + 1> w{};
        if (colour == StarColour::BlackHole) {
            w[0] = 1;
            return w;
        }
        for (const auto roll : kRollNames) {
            const auto n = std::clamp<std::int64_t>(
                cell("class_to_num_satellites", roll, kColourNames[static_cast<std::size_t>(colour)]), 0, kOrbits);
            ++w[static_cast<std::size_t>(n)];
        }
        return w;
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
            s.colour = home ? static_cast<StarColour>(r_.pick(kHomeColour).value_or(4)) : rollColour(s.inNebula, nearHome(i));
            if (home) {
                makeHome(i);
            } else {
                rollSystem(i, satelliteCounts(s.colour));
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
        if (picks[RacePicks::ArtifactsWorld] != 0) {
            p.special = Special::AncientArtifacts;
            s.special = Special::AncientArtifacts;
        }
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


    bool nearHome(int star) const {
        return std::any_of(g_.homes.begin(), g_.homes.end(),
                           [&](const HomeSystem& h) { return h.star != star && distance(star, h.star) < 100.0; });
    }

    bool isHome(int star) const {
        return std::any_of(g_.homes.begin(), g_.homes.end(), [&](const HomeSystem& h) { return h.star == star; });
    }

    // Special finds: a star other than Orion and the home stars, with
    // satellites, gets one when a roll from 0 to 99 is at least
    // star_special_chance; planet_special_chance weighs which. A find that
    // has no place there is rolled again.
    void specials() {
        const auto threshold = p_.integer("star_special_chance");
        std::array<std::int64_t, kSpecialNames.size()> w{};
        for (std::size_t i = 1; i < kSpecialNames.size(); ++i) {
            w[i] = p_.integer("planet_special_chance", {}, kSpecialNames[i]);
        }
        w[static_cast<std::size_t>(Special::Monster)] = 0; // monsters come after
        for (int i = 1; i < static_cast<int>(g_.stars.size()); ++i) {
            Star& s = g_.stars[static_cast<std::size_t>(i)];
            if (isHome(i) || s.satellites() == 0 || s.special != Special::None || s.wormhole != -1) {
                continue;
            }
            // Measured: at the default 64, 41 in 100 such systems get a find
            // in the original, more than the 36 the setting says.
            if (static_cast<std::int64_t>(r_.below(100 * 36)) >= (100 - threshold) * 41) {
                continue;
            }
            for (int tries = 0; tries < 20; ++tries) {
                const auto pick = r_.pick(w);
                if (!pick || placeSpecial(i, static_cast<Special>(*pick))) {
                    break;
                }
            }
        }
    }

    bool placeSpecial(int star, Special special) {
        Star& s = g_.stars[static_cast<std::size_t>(star)];
        switch (special) {
        case Special::Wormhole: {
            std::vector<int> ends;
            for (int j = 1; j < static_cast<int>(g_.stars.size()); ++j) {
                const Star& t = g_.stars[static_cast<std::size_t>(j)];
                if (j != star && !isHome(j) && t.satellites() > 0 && t.special == Special::None && t.wormhole == -1 &&
                    distance(star, j) >= kLayout[layout_].wormholeApart) {
                    ends.push_back(j);
                }
            }
            if (ends.empty()) {
                return false;
            }
            const int end = ends[r_.below(static_cast<std::uint32_t>(ends.size()))];
            s.wormhole = end;
            g_.stars[static_cast<std::size_t>(end)].wormhole = star;
            return true;
        }
        case Special::SpaceDebris:
        case Special::PirateCache:
        case Special::Hero:
            s.special = special;
            return true;
        case Special::GoldDeposits:
        case Special::GemDeposits:
        case Special::Natives:
        case Special::SplinterColony:
        case Special::AncientArtifacts: {
            const bool lively = special == Special::Natives || special == Special::SplinterColony ||
                                special == Special::AncientArtifacts;
            std::vector<int> planets;
            for (const int o : s.orbits) {
                if (o == kNoPlanet) {
                    continue;
                }
                const Planet& q = g_.planets[static_cast<std::size_t>(o)];
                if (q.type == SatelliteType::Planet && (!lively || q.climate >= Climate::Desert)) {
                    planets.push_back(o);
                }
            }
            if (planets.empty()) {
                return false;
            }
            Planet& q = g_.planets[static_cast<std::size_t>(planets[r_.below(static_cast<std::uint32_t>(planets.size()))])];
            q.special = special;
            s.special = special;
            if (lively) {
                q.climate = static_cast<Climate>(static_cast<std::size_t>(Climate::Desert) + r_.pick(kLivelyClimate).value_or(0));
            }
            if ((special == Special::Natives || special == Special::SplinterColony) && q.size < PlanetSize::Medium) {
                q.size = static_cast<PlanetSize>(r_.between(2, 4));
                q.gravity = gravity(q.minerals, q.size);
            }
            return true;
        }
        default:
            return false;
        }
    }

    // The Guardian at Orion; other monsters guard some systems with a find
    // and make their planets better.
    void monsters() {
        if (!g_.stars.empty() && g_.stars[0].special == Special::Orion) {
            g_.monsters.push_back({MonsterKind::Guardian, 0});
        }
        std::vector<int> found;
        for (int i = 1; i < static_cast<int>(g_.stars.size()); ++i) {
            const Special sp = g_.stars[static_cast<std::size_t>(i)].special;
            const Star& s = g_.stars[static_cast<std::size_t>(i)];
            const bool planet = std::any_of(s.orbits.begin(), s.orbits.end(), [&](int o) {
                return o != kNoPlanet && g_.planets[static_cast<std::size_t>(o)].type == SatelliteType::Planet;
            });
            if (sp != Special::None && !isHome(i) && planet) {
                found.push_back(i);
            }
        }
        int guarded = 0;
        for (const int i : found) {
            if (guarded >= kLayout[layout_].monsters || !r_.chance(kGuardPercent)) {
                continue;
            }
            Star& s = g_.stars[static_cast<std::size_t>(i)];
            const auto kind = r_.pick(kGuardKind[static_cast<std::size_t>(s.special)]);
            if (!kind) {
                continue;
            }
            g_.monsters.push_back({static_cast<MonsterKind>(*kind + 1), i});
            ++guarded;
            for (const int o : s.orbits) {
                if (o == kNoPlanet) {
                    continue;
                }
                Planet& q = g_.planets[static_cast<std::size_t>(o)];
                if (q.type != SatelliteType::Planet) {
                    continue;
                }
                // Measured: three in four planets of a guarded system are
                // rich or ultra rich, nearly two in three large or huge and
                // one in three terran or gaia; gravity stays.
                if (q.minerals < Minerals::Rich && r_.chance(65)) {
                    q.minerals = r_.below(2) == 0 ? Minerals::Rich : Minerals::UltraRich;
                }
                if (q.size < PlanetSize::Large && r_.chance(50)) {
                    q.size = r_.below(2) == 0 ? PlanetSize::Large : PlanetSize::Huge;
                }
                if (q.climate < Climate::Terran && r_.chance(30)) {
                    q.climate = r_.below(2) == 0 ? Climate::Terran : Climate::Gaia;
                }
            }
        }
    }

    bool guarded(int star) const {
        return std::any_of(g_.monsters.begin(), g_.monsters.end(), [&](const Monster& m) { return m.star == star; });
    }

    bool growsFood(const Planet& q) const {
        return q.type == SatelliteType::Planet &&
               p_.integer("food_farmer_base", {}, kClimateNames[static_cast<std::size_t>(q.climate)]) > 0;
    }

    // A home system of a race that eats (not Lithovore) has a planet that
    // grows food within six parsecs, outside the home systems and the
    // systems monsters guard. When it has none, a planet of a system in reach,
    // picked at random, gets a climate that grows food; a system without
    // planets gets one. False when no system is in reach.
    bool foodWorlds() {
        const double reach = kFoodWorldParsecs * kParsec;
        for (std::size_t i = 0; i < g_.homes.size(); ++i) {
            if (s_.players[i].picks[RacePicks::Lithovore] != 0) {
                continue;
            }
            const int home = g_.homes[i].star;
            std::vector<int> inReach;
            bool fed = false;
            for (int j = 1; j < static_cast<int>(g_.stars.size()) && !fed; ++j) {
                const Star& t = g_.stars[static_cast<std::size_t>(j)];
                if (isHome(j) || guarded(j) || t.colour == StarColour::BlackHole || distance(home, j) > reach) {
                    continue;
                }
                inReach.push_back(j);
                fed = std::any_of(t.orbits.begin(), t.orbits.end(), [&](int o) {
                    return o != kNoPlanet && growsFood(g_.planets[static_cast<std::size_t>(o)]);
                });
            }
            if (fed) {
                continue;
            }
            if (inReach.empty()) {
                return false;
            }
            feed(inReach[r_.below(static_cast<std::uint32_t>(inReach.size()))]);
        }
        return true;
    }

    void feed(int star) {
        Star& s = g_.stars[static_cast<std::size_t>(star)];
        std::vector<int> planets, others;
        std::vector<std::size_t> empty;
        for (std::size_t o = 0; o < kOrbits; ++o) {
            if (s.orbits[o] == kNoPlanet) {
                empty.push_back(o);
            } else if (g_.planets[static_cast<std::size_t>(s.orbits[o])].type == SatelliteType::Planet) {
                planets.push_back(s.orbits[o]);
            } else {
                others.push_back(s.orbits[o]);
            }
        }
        int target = kNoPlanet;
        if (!planets.empty()) {
            target = planets[r_.below(static_cast<std::uint32_t>(planets.size()))];
        } else if (!empty.empty()) {
            const auto orbit = empty[r_.below(static_cast<std::uint32_t>(empty.size()))];
            target = add(rollPlanet(star, static_cast<int>(orbit)));
            s.orbits[orbit] = target;
        } else {
            target = others[r_.below(static_cast<std::uint32_t>(others.size()))];
            Planet& old = g_.planets[static_cast<std::size_t>(target)];
            Planet p = rollPlanet(star, old.orbit);
            p.special = old.special;
            old = p;
        }
        Planet& q = g_.planets[static_cast<std::size_t>(target)];
        // The climate the orbit gives, rolled until it grows food.
        const auto colour = std::min<std::size_t>(static_cast<std::size_t>(s.colour), 5);
        const auto group = static_cast<std::size_t>(std::clamp<std::int64_t>(
            cell("orbit_to_climate_group", kColourNames[colour], kOrbitNames[static_cast<std::size_t>(q.orbit)]), 0,
            3));
        for (int k = 0; k < 100 && !growsFood(q); ++k) {
            q.climate = rollClimate(group);
        }
        if (!growsFood(q)) { // the orbit's climates grow none: the best
            std::int64_t best = 0;
            for (std::size_t c = 0; c < kClimates; ++c) {
                const auto food = p_.integer("food_farmer_base", {}, kClimateNames[c]);
                if (food > best) {
                    best = food;
                    q.climate = static_cast<Climate>(c);
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
    const std::vector<Canvas>& nebulaPictures_;
    const GalaxySettings& s_;
    Random& r_;
    std::size_t map_ = 1;    // the row of map_generation
    std::size_t stars_ = 1;  // the column of stars_in_galaxy
    std::size_t layout_ = 1; // of kLayout
    std::size_t age_ = 1;
    Galaxy g_;
};

} // namespace

std::vector<Canvas> loadNebulae(GameData& data) {
    const LbxArchive& lbx = data.archive("STARBG.LBX");
    std::vector<Canvas> out;
    for (std::size_t i = 0; i < kNebulaPictures; ++i) {
        const Image image = Image::parse(lbx.entry(kFirstNebula + 4 * i));
        Canvas c(image.width(), image.height(), 0);
        image.draw(0, c, 0, 0);
        out.push_back(std::move(c));
    }
    return out;
}

GalaxyGenerator::GalaxyGenerator(const Parameters& parameters, StarNames names, std::vector<Canvas> nebulae)
    : p_(parameters), names_(std::move(names)), nebulae_(std::move(nebulae)) {}

Galaxy GalaxyGenerator::generate(const GalaxySettings& settings, Random& random) const {
    return Maker(p_, names_, nebulae_, settings, random).make();
}

} // namespace hegemon
