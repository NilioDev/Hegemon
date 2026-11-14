// SPDX-License-Identifier: GPL-3.0-or-later
#include "tech_list.hpp"
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/tech.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

const std::string kList = R"({"patch_version": "test", "parameters": [)" + test::techSettings() + "]}";

} // namespace

HGM_TEST("Research: the tree's chains and what a new empire knows") {
    const Parameters p = Parameters::defaults(kList);
    const TechTree tree(p);
    HGM_CHECK(tree.fieldCount() == 19 && tree.techCount() == 5);
    HGM_CHECK(tree.first(TechCategory::Construction) == 4 && tree.field(4).next == 13);
    HGM_CHECK(tree.field(13).category == static_cast<int>(TechCategory::Construction));
    HGM_CHECK((tree.field(13).techs == std::vector<int>{3, 4}) && tree.techField(2) == 4 && tree.techField(9) == -1);
    const Knowledge k = tree.averageStart();
    HGM_CHECK(k.knowsField(0) && k.knowsField(4) && !k.knowsField(13) && !k.knowsField(1));
    HGM_CHECK(k.knowsTech(1) && k.knowsTech(2) && !k.knowsTech(3));
    HGM_CHECK(tree.researchable(k, TechCategory::Construction) == 13);
    HGM_CHECK(tree.researchable(k, TechCategory::Biology) == 1);
    Knowledge all = k;
    tree.learn(all, 13, tree.field(13).techs);
    HGM_CHECK(tree.researchable(all, TechCategory::Construction) == 0 && all.knowsTech(4));
}

HGM_TEST("Research: the panel's estimate counts to one and a half times the cost") {
    // Measured: 80 RP at 10 a turn shows ~11 turns after the first turn.
    HGM_CHECK(researchTurns(80, 10, 10) == 11);
    HGM_CHECK(researchTurns(80, 20, 10) == 10);
    HGM_CHECK(researchTurns(80, 0, 7) == 18);
    HGM_CHECK(researchTurns(80, 200, 10) == 1 && researchTurns(80, 0, 0) == 1);
}

HGM_TEST("Research: the original's tree offers the measured fields at the start") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const TechTree tree(data.parameters());
    const TechNames names(data.archive("TECHNAME.LBX").entry(0), tree);
    const Knowledge k = tree.averageStart();
    struct Expected {
        TechCategory category;
        const char* name;
        int cost;
        std::vector<std::string> apps;
    };
    const Expected expected[] = {
        {TechCategory::Construction, "Advanced Engineering", 80, {"Anti-Missile Rockets", "Reinforced Hull", "Fighter Bays"}},
        {TechCategory::Power, "Advanced Fusion", 250, {"Augmented Engines", "Fusion Bomb", "Fusion Drive"}},
        {TechCategory::Chemistry, "Advanced Metallurgy", 250, {"Deuterium Fuel Cells", "Tritanium Armor"}},
        {TechCategory::Sociology, "Military Tactics", 150, {"Space Academy"}},
        {TechCategory::Computers, "Optronics", 150, {"Dauntless Guidance System", "Optronic Computer", "Research Laboratory"}},
        {TechCategory::Biology, "Astro Biology", 80, {"Biospheres", "Hydroponic Farm"}},
        {TechCategory::Physics, "Fusion Physics", 150, {"Fusion Beam", "Fusion Rifle"}},
        {TechCategory::ForceFields, "Advanced Magnetism", 250, {"Class I Shield", "Ecm Jammer", "Mass Driver"}},
    };
    for (const auto& e : expected) {
        const int f = tree.researchable(k, e.category);
        HGM_CHECK(names.field(f) == e.name && tree.field(f).cost == e.cost);
        std::vector<std::string> apps;
        for (const int t : tree.field(f).techs) {
            apps.push_back(names.tech(t));
        }
        HGM_CHECK(apps == e.apps);
    }
    HGM_CHECK(names.category(TechCategory::Construction) == "Construction" && names.category(TechCategory::ForceFields) == "Fields");
    // Measured (the game's own field states): the pre-warp fields are known.
    for (const char* known : {"engineering", "nuclear_fission", "physics", "chemistry", "electronics", "cold_fusion"}) {
        bool found = false;
        for (std::size_t i = 0; i < tree.fieldCount(); ++i) {
            if (tree.field(static_cast<int>(i)).key == known) {
                found = k.knowsField(static_cast<int>(i));
            }
        }
        HGM_CHECK(found);
    }
    HGM_CHECK(names.tech(tree.field(0).techs.at(0)) == "Capitol");
}
