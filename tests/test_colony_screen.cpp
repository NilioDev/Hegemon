// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/colony_screen.hpp"

#include <filesystem>

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
