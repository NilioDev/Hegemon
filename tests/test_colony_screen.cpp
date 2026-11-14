// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/colony_screen.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Messages: blanks filled as the original's printf fills them") {
    HGM_CHECK(formatMessage("Pop %d,%03d k (%+dk)", {8LL, 73LL, 73LL}) == "Pop 8,073 k (+73k)");
    HGM_CHECK(formatMessage("%sColony of %s", {std::string(), std::string("Sol Prime")}) == "Colony of Sol Prime");
    HGM_CHECK(formatMessage("%+d %d %3d%%", {-4LL, 0LL, 7LL}) == "-4 0   7%");
    // Missing or wrong values leave the blank empty.
    HGM_CHECK(formatMessage("a%db%s", {std::string("x")}) == "ab");
}

HGM_TEST("Colony screen: the title, the population and Return") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Races races(data);
    Galaxy g;
    Star s;
    s.name = "Altair";
    g.stars.push_back(s);
    Planet p;
    p.climate = Climate::Terran;
    p.image = 2;
    g.planets.push_back(p);
    g.homes.push_back({0, 0, 0});
    const Game game = startGame(data.parameters(), g, {races.stock(0)}, {0});
    const Economy economy(data.parameters());
    ColonyScreen screen(data, game, economy, 0);
    // Measured on the Alkari's home colony.
    HGM_CHECK(screen.title() == "Colony of Altair Prime");
    HGM_CHECK(screen.population() == "Pop 8,000 k (+73k)");
    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    int title = 0;
    for (int y = 0; y < 14; ++y) {
        for (int x = 229; x < 411; ++x) {
            title += canvas.at(x, y) == 253 ? 1 : 0;
        }
    }
    HGM_CHECK(title > 400);
    HGM_CHECK((palette.colors[253] == Color{109, 105, 142}));
    screen.press(600, 470);
    HGM_CHECK(screen.release(600, 470));
    screen.press(600, 470);
    HGM_CHECK(!screen.release(100, 100));
}

HGM_TEST("Colony screen: the icons' places in a row") {
    // Measured: 20 pixels apart while 150 pixels hold them, closer after.
    HGM_CHECK(ColonyScreen::iconStep(1) == 20 && ColonyScreen::iconStep(8) == 20);
    HGM_CHECK(ColonyScreen::iconStep(9) == 18 && ColonyScreen::iconStep(10) == 16);
    HGM_CHECK(ColonyScreen::iconStep(11) == 15 && ColonyScreen::iconStep(12) == 13);
}

HGM_TEST("Colony screen: the rows of money, food, production and research") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Races races(data);
    const Economy economy(data.parameters());
    const auto rows = [&](std::size_t race, PlanetSize size, Special special) {
        Galaxy g;
        g.stars.push_back(Star{});
        Planet p;
        p.size = size;
        p.climate = Climate::Terran;
        p.special = special;
        g.planets.push_back(p);
        g.homes.push_back({static_cast<int>(race), 0, 0});
        const Game game = startGame(data.parameters(), g, {races.stock(race)}, {0});
        const ColonyScreen screen(data, game, economy, 0);
        std::vector<std::vector<int>> r;
        for (std::size_t i = 0; i < ColonyScreen::kRows; ++i) {
            r.push_back(screen.row(static_cast<ColonyScreen::Row>(i)));
        }
        return r;
    };
    using V = std::vector<int>;
    constexpr int G = ColonyScreen::kGap;
    // Measured on the home colonies after the first turn: the buildings'
    // upkeep, then the income; the food eaten, then what's left over; the
    // production, then what pollution takes; tens first.
    const auto alkari = rows(0, PlanetSize::Medium, Special::AncientArtifacts);
    HGM_CHECK((alkari[0] == V{3, 3, 3, G, 3, 3, 3, 3, 3, 3, 3, 3}));
    HGM_CHECK((alkari[1] == V(8, 0)) && (alkari[2] == V(6, 1)) && (alkari[3] == V{6}));
    const auto klackon = rows(6, PlanetSize::Large, Special::None);
    HGM_CHECK((klackon[0] == V{3, 3, 3, G, 7}));
    HGM_CHECK((klackon[1] == V{0, 0, 0, 0, 0, 0, 0, 0, G, 0}));
    HGM_CHECK((klackon[2] == V{5, 9, 9}) && (klackon[3] == V{6, 2, 2}));
    // A cybernetic race eats production too.
    const auto meklar = rows(7, PlanetSize::Medium, Special::None);
    HGM_CHECK((meklar[1] == V{0, 0, 0, 0, 1, 1, 1, 1, G}));
    HGM_CHECK((meklar[2] == V{1, 1, 1, 1, 9, 9}) && (meklar[3] == V{6, 2, 2}));
    // A lithovore eats nothing.
    HGM_CHECK(rows(11, PlanetSize::Medium, Special::None)[1].empty());
}
