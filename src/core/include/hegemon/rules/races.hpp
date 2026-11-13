// SPDX-License-Identifier: GPL-3.0-or-later
//
// The thirteen stock races and their racial picks, read from RACESTUF.LBX
// and changed by the fan patch's `stock_race` settings. See
// docs/formats/races.md.
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

class FanPatchConfig;
class GameData;

// A race's picks, one number per pick, in the order of RACESTUF.LBX entry 7.
struct RacePicks {
    enum Pick : std::size_t {
        Government, // 0 feudal, 2 dictatorship, 4 democracy, 6 unification
        Growth,     // the nine picks with levels: 0 none, 1 the bad one,
        Farming,    // 2 and 3 the two good ones
        Industry,
        Science,
        Money,
        Defense,
        Attack,
        Ground,
        Spying,
        LowGravity, // the rest: 0 or 1
        HighGravity,
        Aquatic,
        Subterranean,
        LargeHomeWorld,
        HomeWorldRichness, // 0 normal, 1 rich, 2 poor
        ArtifactsWorld,
        Cybernetic,
        Lithovore,
        Repulsive,
        Charismatic,
        Uncreative,
        Creative,
        Tolerant,
        FantasticTraders,
        Telepathic,
        Lucky,
        Omniscient,
        StealthyShips,
        TransDimensional,
        Warlord,
        Count
    };
    std::array<std::int8_t, Count> values{};

    int operator[](Pick p) const { return values[p]; }
    bool operator==(const RacePicks&) const = default;
};

class Races {
public:
    static constexpr std::size_t kStockRaces = 13;
    // The names the fan patch's settings use, in the original's order.
    static constexpr std::array<std::string_view, kStockRaces> kNames = {
        "Alkari", "Bulrathi", "Darloks", "Elerians", "Gnolams", "Humans", "Klackons",
        "Meklars", "Mrrshan", "Psilons", "Sakkra", "Silicoids", "Trilarians"};

    // Reads the stock races and the English texts of RACESTUF.LBX and
    // applies `stock_race` settings. Throws FormatError when the archive is
    // damaged and ConfigError when a setting makes no sense.
    explicit Races(GameData& data);
    // The same from the archive's entries, for tests.
    Races(std::span<const std::uint8_t> table, std::span<const std::uint8_t> levels,
          std::span<const std::uint8_t> labels, const FanPatchConfig* config);

    const RacePicks& stock(std::size_t race) const { return stock_.at(race); }

    // What the race selection shows under the portrait, such as
    // "Ship Defense: +50, Artifacts Home World, Dictatorship".
    std::string description(const RacePicks& picks) const;

    // The value of a pick level in tenths, as the fan patch has it by
    // default (`race_pick` settings are not read yet): growth −50 % at
    // level 1, +50 % at 2 and +100 % at 3, and so on.
    static int levelValue(RacePicks::Pick pick, int level);

private:
    void applyConfig(const FanPatchConfig& config);

    std::vector<RacePicks> stock_;
    std::vector<std::string> governments_; // Feudal, Dictatorship, Democracy, Unification
    std::vector<std::string> labels_;      // entry 8: "government", "Population Growth:", ...
};

} // namespace hegemon
