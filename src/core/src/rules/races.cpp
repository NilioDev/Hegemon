// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/races.hpp"

#include "hegemon/config/fan_patch_config.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>
#include <cctype>

namespace hegemon {

namespace {

// RACESTUF.LBX: English texts in entries 0 and 8, the stock races in 7.
constexpr const char* kArchive = "RACESTUF.LBX";
constexpr std::size_t kLevelsEntry = 0;
constexpr std::size_t kTableEntry = 7;
constexpr std::size_t kLabelsEntry = 8;
// In entry 0, the four governments follow the word "Governments".
constexpr std::size_t kFirstGovernment = 37;
constexpr std::size_t kGovernments = 4;
// In entry 8, the label of a pick is at its number; "Poor Home World" last.
constexpr std::size_t kPoorHomeWorldLabel = 31;
constexpr std::size_t kLabels = 32;

std::vector<std::string> zeroSeparated(std::span<const std::uint8_t> bytes) {
    std::vector<std::string> out;
    std::string current;
    for (const auto b : bytes) {
        if (b == 0) {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(static_cast<char>(b));
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

std::string lower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// The pick names of the `stock_race` setting, in table order.
constexpr std::array<std::string_view, RacePicks::Count> kPickNames = {
    "government", "growth", "farming", "industry", "science", "money", "defense", "attack",
    "ground", "spying", "lowg_world", "highg_world", "aquatic", "subterranean", "large_hw",
    "hw_richness", "arti_world", "cybernetic", "lithovore", "repulsive", "charismatic",
    "uncreative", "creative", "tolerant", "fantastic_traders", "telepathic", "lucky",
    "omniscient", "stealthy_ships", "trans_dimensional", "warlord"};

constexpr std::array<std::string_view, kGovernments> kGovernmentNames = {"feudal", "dictatorship", "democracy",
                                                                          "unification"};

// A pick's value in a setting: a number, perhaps after a letter prefix as
// in "sd3", or a government's name.
std::int8_t settingValue(RacePicks::Pick pick, const ConfigValue& v, const ConfigEntry& e) {
    const auto fail = [&] {
        return ConfigError(e.file + ":" + std::to_string(e.line) + ": stock_race value makes no sense");
    };
    if (pick == RacePicks::Government) {
        const auto it = std::find(kGovernmentNames.begin(), kGovernmentNames.end(), lower(v.text));
        if (it == kGovernmentNames.end()) {
            throw fail();
        }
        return static_cast<std::int8_t>(2 * (it - kGovernmentNames.begin()));
    }
    std::int64_t n = 0;
    if (v.kind == ConfigValue::Kind::Number) {
        n = v.number;
    } else {
        std::size_t at = 0;
        while (at < v.text.size() && std::isalpha(static_cast<unsigned char>(v.text[at]))) {
            ++at;
        }
        const std::string digits = v.text.substr(at);
        if (digits.empty() || digits.size() > 3 ||
            !std::all_of(digits.begin() + (digits[0] == '-' ? 1 : 0), digits.end(),
                         [](char c) { return std::isdigit(static_cast<unsigned char>(c)); }) ||
            digits == "-") {
            throw fail();
        }
        n = std::stoll(digits);
    }
    if (pick == RacePicks::HomeWorldRichness && n == -1) {
        n = 2; // poor
    }
    const std::int64_t max = pick <= RacePicks::Spying ? 3 : pick == RacePicks::HomeWorldRichness ? 2 : 1;
    if (n < 0 || n > max) {
        throw fail();
    }
    return static_cast<std::int8_t>(n);
}

std::string formatTenths(int tenths) {
    std::string out = tenths < 0 ? "-" : "+";
    const int a = tenths < 0 ? -tenths : tenths;
    out += std::to_string(a / 10);
    if (a % 10 != 0) {
        out += "." + std::to_string(a % 10);
    }
    return out;
}

} // namespace

Races::Races(GameData& data)
    : Races(data.archive(kArchive).entry(kTableEntry), data.archive(kArchive).entry(kLevelsEntry),
            data.archive(kArchive).entry(kLabelsEntry), data.config()) {}

Races::Races(std::span<const std::uint8_t> table, std::span<const std::uint8_t> levels,
             std::span<const std::uint8_t> labels, const FanPatchConfig* config) {
    Reader r(table);
    const std::uint16_t count = r.u16();
    const std::uint16_t size = r.u16();
    if (count != kStockRaces || size != RacePicks::Count) {
        throw FormatError("race table is not 13 races of 31 picks");
    }
    for (std::size_t i = 0; i < count; ++i) {
        RacePicks p;
        for (std::size_t k = 0; k < RacePicks::Count; ++k) {
            p.values[k] = static_cast<std::int8_t>(r.u8());
        }
        stock_.push_back(p);
    }
    const auto words = zeroSeparated(levels);
    if (words.size() < kFirstGovernment + kGovernments) {
        throw FormatError("race texts too short");
    }
    governments_.assign(words.begin() + kFirstGovernment, words.begin() + kFirstGovernment + kGovernments);
    labels_ = zeroSeparated(labels);
    if (labels_.size() < kLabels) {
        throw FormatError("race pick labels too short");
    }
    if (config != nullptr) {
        applyConfig(*config);
    }
}

void Races::applyConfig(const FanPatchConfig& config) {
    for (const auto* e : config.startingWith({"stock_race"})) {
        if (e->keys.size() < 2 || e->values.empty()) {
            continue;
        }
        const auto race = std::find_if(kNames.begin(), kNames.end(),
                                       [&](std::string_view n) { return lower(n) == e->keys[1]; });
        if (race == kNames.end()) {
            throw ConfigError(e->file + ":" + std::to_string(e->line) + ": stock_race names no stock race");
        }
        RacePicks& picks = stock_[static_cast<std::size_t>(race - kNames.begin())];
        if (e->keys.size() == 3) {
            // stock_race <race> <pick> = <value>;
            const auto pick = std::find(kPickNames.begin(), kPickNames.end(), e->keys[2]);
            if (pick == kPickNames.end()) {
                throw ConfigError(e->file + ":" + std::to_string(e->line) + ": stock_race names no pick");
            }
            const auto p = static_cast<RacePicks::Pick>(pick - kPickNames.begin());
            picks.values[p] = settingValue(p, e->values[0], *e);
        } else if (e->keys.size() == 2 && e->values.size() == RacePicks::Count) {
            // stock_race <race> = <government> <30 values>;
            for (std::size_t k = 0; k < RacePicks::Count; ++k) {
                const auto p = static_cast<RacePicks::Pick>(k);
                picks.values[k] = settingValue(p, e->values[k], *e);
            }
        } else {
            throw ConfigError(e->file + ":" + std::to_string(e->line) + ": stock_race needs a pick or all 31");
        }
    }
}

int Races::levelValue(RacePicks::Pick pick, int level) {
    // Tenths for levels 1 (the bad one), 2 and 3.
    static constexpr std::array<std::array<int, 3>, 9> kValues = {{
        {-500, 500, 1000}, // growth, in percent
        {-5, 10, 20},      // farming
        {-10, 10, 20},     // industry
        {-10, 10, 20},     // science
        {-5, 5, 10},       // money
        {-200, 250, 500},  // defense
        {-200, 200, 500},  // attack
        {-100, 100, 200},  // ground combat
        {-100, 100, 200},  // spying
    }};
    if (pick < RacePicks::Growth || pick > RacePicks::Spying || level < 1 || level > 3) {
        return 0;
    }
    return kValues[pick - RacePicks::Growth][static_cast<std::size_t>(level - 1)];
}

std::string Races::description(const RacePicks& picks) const {
    std::vector<std::string> parts;
    for (std::size_t k = RacePicks::Growth; k < RacePicks::Count; ++k) {
        const auto p = static_cast<RacePicks::Pick>(k);
        const int v = picks[p];
        if (v == 0) {
            continue;
        }
        if (p <= RacePicks::Spying) {
            std::string label = labels_[k];
            if (label.empty() || label.back() != ':') {
                label += ':'; // "Industrial Production" has lost its colon
            }
            parts.push_back(label + " " + formatTenths(levelValue(p, v)));
        } else if (p == RacePicks::HomeWorldRichness) {
            parts.push_back(labels_[v == 2 ? kPoorHomeWorldLabel : k]);
        } else {
            parts.push_back(labels_[k]);
        }
    }
    const auto government = static_cast<std::size_t>(picks[RacePicks::Government] / 2);
    if (government < governments_.size()) {
        parts.push_back(governments_[government]);
    }
    std::string out;
    for (const auto& part : parts) {
        out += (out.empty() ? "" : ", ") + part;
    }
    return out;
}

} // namespace hegemon
