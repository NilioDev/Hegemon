// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/fan_patch_config.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Pick = hegemon::RacePicks::Pick;

namespace {

std::vector<std::uint8_t> words(std::initializer_list<std::string> list) {
    std::vector<std::uint8_t> out;
    for (const auto& w : list) {
        out.insert(out.end(), w.begin(), w.end());
        out.push_back(0);
    }
    return out;
}

// Entry 0 up to the governments: 36 words, "Governments", then the four.
std::vector<std::uint8_t> levelTexts() {
    std::vector<std::string> list(36, "x");
    list.push_back("Governments");
    for (const char* g : {"Feudal", "Dictatorship", "Democracy", "Unification"}) {
        list.push_back(g);
    }
    std::vector<std::uint8_t> out;
    for (const auto& w : list) {
        out.insert(out.end(), w.begin(), w.end());
        out.push_back(0);
    }
    return out;
}

std::vector<std::uint8_t> labelTexts() {
    return words({"government", "Population Growth:", "Food Production:", "Industrial Production",
                  "Scientific Research:", "Tax:", "Ship Defense:", "Ship Offense:", "Ground Combat:", "Espionage:",
                  "Low Gravity", "High Gravity", "Aquatic", "Subterranean", "Large Home World", "Rich Home World",
                  "Artifacts Home World", "Cybernetic", "Lithovore", "Repulsive", "Charismatic", "Uncreative",
                  "Creative", "Tolerant", "Expert Traders", "Telepathic", "Lucky", "Omniscient", "Stealth Ships",
                  "Trans Dimensional", "Warlord", "Poor Home World"});
}

// 13 races of 31 picks: all dictatorships with nothing else, but race 0.
std::vector<std::uint8_t> table() {
    std::vector<std::uint8_t> out = {13, 0, 31, 0};
    for (int r = 0; r < 13; ++r) {
        for (int p = 0; p < 31; ++p) {
            out.push_back(static_cast<std::uint8_t>(p == 0 ? 2 : 0));
        }
    }
    out[4 + 1] = 1;  // race 0: growth -50
    out[4 + 3] = 3;  // industry +2
    out[4 + 15] = 2; // poor home world
    out[4 + 30] = 1; // warlord
    return out;
}

template <typename F> bool throws(F&& f) {
    try {
        f();
    } catch (const hegemon::FormatError&) {
        return true;
    }
    return false;
}

} // namespace

HGM_TEST("Races: picks from the table, described as the race selection shows them") {
    const auto t = table();
    const auto levels = levelTexts();
    const auto labels = labelTexts();
    const hegemon::Races races(t, levels, labels, nullptr);
    HGM_CHECK(races.stock(0)[Pick::Growth] == 1);
    HGM_CHECK(races.description(races.stock(0)) ==
              "Population Growth: -50, Industrial Production: +2, Poor Home World, Warlord, Dictatorship");
    HGM_CHECK(races.description(races.stock(1)) == "Dictatorship");
    HGM_CHECK(hegemon::Races::levelValue(Pick::Money, 2) == 5);
    HGM_CHECK(hegemon::Races::levelValue(Pick::Defense, 1) == -200);

    // Damaged tables are refused.
    auto shortTable = t;
    shortTable.resize(100);
    HGM_CHECK(throws([&] { (void)hegemon::Races(shortTable, levels, labels, nullptr); }));
    auto wrongSize = t;
    wrongSize[2] = 30;
    HGM_CHECK(throws([&] { (void)hegemon::Races(wrongSize, levels, labels, nullptr); }));
    HGM_CHECK(throws([&] { (void)hegemon::Races(t, words({"a", "b"}), labels, nullptr); }));
}

HGM_TEST("Races: the fan patch's stock_race settings change them") {
    const fs::path folder = fs::temp_directory_path() / "hegemon_test_races";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::ofstream(folder / "ORION2.CFG") << "stock_race Humans government = democracy;\n"
                                         << "stock_race Alkari growth = 0;\n"
                                         << "stock_race bulrathi = feudal g2 f0 i0 s0 m0 sd3 sa0 gc0 spy0"
                                         << " 0 0 0 0 1 -1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1;\n";
    const auto config = hegemon::FanPatchConfig::load(folder);
    const hegemon::Races races(table(), levelTexts(), labelTexts(), &config);
    HGM_CHECK(races.stock(5)[Pick::Government] == 4);
    HGM_CHECK(races.stock(0)[Pick::Growth] == 0);
    HGM_CHECK(races.description(races.stock(1)) ==
              "Population Growth: +50, Ship Defense: +50, Large Home World, Poor Home World, Warlord, Feudal");

    for (const char* bad : {"stock_race Humans government = anarchy;\n", "stock_race Orions growth = 1;\n",
                            "stock_race Humans speed = 1;\n", "stock_race Humans growth = 4;\n",
                            "stock_race Humans = feudal 1 2 3;\n"}) {
        std::ofstream(folder / "ORION2.CFG") << bad;
        const auto c = hegemon::FanPatchConfig::load(folder);
        HGM_CHECK(throws([&] { (void)hegemon::Races(table(), levelTexts(), labelTexts(), &c); }));
    }
    fs::remove_all(folder);
}

HGM_TEST("Races: the player's stock races, described as the original shows them") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    const hegemon::Races races(data);
    // Read off the original's race selection with fan patch 1.51.
    const std::vector<std::string> expected = {
        "Ship Defense: +50, Artifacts Home World, Dictatorship",
        "Ship Offense: +20, Ground Combat: +10, High Gravity, Dictatorship",
        "Espionage: +20, Stealth Ships, Dictatorship",
        "Ship Defense: +25, Ship Offense: +20, Telepathic, Omniscient, Feudal",
        "Tax: +1, Low Gravity, Expert Traders, Lucky, Dictatorship",
        "Charismatic, Democracy",
        "Food Production: +1, Industrial Production: +1, Large Home World, Uncreative, Unification",
        "Industrial Production: +2, Cybernetic, Dictatorship",
        "Ship Offense: +50, Rich Home World, Warlord, Dictatorship",
        "Scientific Research: +2, Low Gravity, Large Home World, Creative, Dictatorship",
        "Population Growth: +100, Food Production: +1, Espionage: -10, Subterranean, Large Home World, Feudal",
        "Population Growth: -50, Lithovore, Repulsive, Tolerant, Dictatorship",
        "Aquatic, Trans Dimensional, Dictatorship",
    };
    for (std::size_t r = 0; r < hegemon::Races::kStockRaces; ++r) {
        HGM_CHECK(races.description(races.stock(r)) == expected[r]);
    }
}
