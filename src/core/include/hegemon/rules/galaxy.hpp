// SPDX-License-Identifier: GPL-3.0-or-later
//
// A galaxy: its stars, the planets, asteroid belts and gas giants around
// them, and where each player starts. See docs/rules/galaxy.md.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

// The numbers are the fan patch's, as its Lua scripts see them.
enum class StarColour : std::uint8_t { Blue, White, Yellow, Orange, Red, Brown, BlackHole };
inline constexpr std::size_t kStarColours = 7;

enum class SatelliteType : std::uint8_t { AsteroidBelt = 1, GasGiant = 2, Planet = 3 };
enum class PlanetSize : std::uint8_t { Tiny, Small, Medium, Large, Huge };
enum class Minerals : std::uint8_t { UltraPoor, Poor, Abundant, Rich, UltraRich };
enum class Gravity : std::uint8_t { Low, Normal, Heavy };
enum class Climate : std::uint8_t { Toxic, Radiated, Barren, Desert, Tundra, Ocean, Swamp, Arid, Terran, Gaia };
inline constexpr std::size_t kClimates = 10;

// What a star system or a planet has besides its stars and planets, in the
// order of the fan patch's planet_special_chance table.
enum class Special : std::uint8_t {
    None,
    Wormhole,
    SpaceDebris,
    PirateCache,
    GoldDeposits,
    GemDeposits,
    Natives,
    SplinterColony,
    Hero,
    Monster,
    AncientArtifacts,
    Orion,
};

inline constexpr std::size_t kOrbits = 5;
inline constexpr int kNoPlanet = -1;

struct Planet {
    int star = 0;
    int orbit = 0; // 0 is closest to the star
    SatelliteType type = SatelliteType::Planet;
    PlanetSize size = PlanetSize::Medium;
    Minerals minerals = Minerals::Abundant;
    Gravity gravity = Gravity::Normal;
    Climate climate = Climate::Barren;
    Special special = Special::None;
};

struct Star {
    std::string name;
    int x = 0, y = 0; // on the galaxy map, in its own units
    StarColour colour = StarColour::Yellow;
    Special special = Special::None;
    int wormhole = -1; // the star at the other end, or -1
    bool inNebula = false;
    std::array<int, kOrbits> orbits{kNoPlanet, kNoPlanet, kNoPlanet, kNoPlanet, kNoPlanet};

    int satellites() const;
};

enum class MonsterKind : std::uint8_t { Guardian, Amoeba, Crystal, Dragon, Eel, Hydra };

// A space monster guarding a star system: the Guardian at Orion, the others
// at systems with a special find.
struct Monster {
    MonsterKind kind = MonsterKind::Hydra;
    int star = 0;
};

// A nebula: one of the twelve pictures, its top left corner on the map.
struct Nebula {
    int picture = 0;
    int x = 0, y = 0;
};

// Where a player starts.
struct HomeSystem {
    int race = 0; // one of the stock races
    int star = 0;
    int planet = 0;
};

struct Galaxy {
    int width = 0, height = 0;
    bool cluster = false;
    std::vector<Star> stars; // Orion first, unless it was left out
    std::vector<Planet> planets;
    std::vector<HomeSystem> homes; // one per player, the human first
    std::vector<Monster> monsters;
    std::vector<Nebula> nebulae;
};

} // namespace hegemon
