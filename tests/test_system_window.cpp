// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/system_window.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The measured Sal system: a large ocean planet in the second orbit, a
// huge barren one in the fifth, a wormhole to Derke.
Game salGame() {
    Game game;
    Star sal;
    sal.name = "Sal";
    sal.colour = StarColour::Red;
    sal.wormhole = 1;
    sal.orbits = {kNoPlanet, 0, kNoPlanet, kNoPlanet, 1};
    Star derke;
    derke.name = "Derke";
    game.galaxy.stars = {sal, derke};
    Planet ocean;
    ocean.orbit = 1;
    ocean.size = PlanetSize::Large;
    ocean.climate = Climate::Ocean;
    Planet barren;
    barren.orbit = 4;
    barren.size = PlanetSize::Huge;
    barren.climate = Climate::Barren;
    game.galaxy.planets = {ocean, barren};
    Player human;
    human.explored = {true, false};
    game.players = {human};
    return game;
}

} // namespace

HGM_TEST("Star system window: the measured Sal system") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    Game game = salGame();
    SystemWindow corner(data, game, 0, SystemWindow::Place::MapCorner, 0);
    HGM_CHECK(corner.title() == "Star System Sal");
    // Measured: "Stable Wormhole" while Derke is unexplored.
    HGM_CHECK(corner.wormholeText() == "Stable Wormhole");
    // Measured: the frame at (22, 22), Close at (285, 258).
    HGM_CHECK(corner.area().x == 22 && corner.area().y == 22);
    HGM_CHECK(corner.closeButton().x == 285 && corner.closeButton().y == 258);
    HGM_CHECK(corner.planetRect(0).has_value() && corner.planetRect(1).has_value());
    const Rect ocean = *corner.planetRect(0);
    HGM_CHECK(ocean.w == 23);
    HGM_CHECK(corner.planetAt(ocean.centerX(), ocean.centerY()) == 0);
    const Rect close = corner.closeButton();
    corner.press(close.centerX(), close.centerY());
    HGM_CHECK(corner.release(close.centerX(), close.centerY()) == SystemWindow::Action::Close);

    game.players[0].explored[1] = true;
    SystemWindow arrival(data, game, 0, SystemWindow::Place::Middle, 0, "Scouts arrive at the Sal system");
    HGM_CHECK(arrival.wormholeText() == "Wormhole links Derke");
    HGM_CHECK(arrival.area().x == 147 && arrival.area().y == 104);
    Canvas canvas(640, 480, 0);
    Palette palette;
    arrival.draw(canvas, palette);
    // The title in blue (colour 65) in the bar, the wormhole in green
    // (115) at the bottom right.
    int blue = 0, green = 0;
    for (int y = 123; y < 138; ++y) {
        for (int x = 194; x < 445; ++x) {
            blue += canvas.at(x, y) == 65 ? 1 : 0;
        }
    }
    for (int y = 323; y < 330; ++y) {
        for (int x = 400; x < 476; ++x) {
            green += canvas.at(x, y) == 115 ? 1 : 0;
        }
    }
    HGM_CHECK(blue > 300);
    HGM_CHECK(green > 50);

    // An unexplored star shows no planets.
    game.players[0].explored[0] = false;
    SystemWindow unknown(data, game, 0, SystemWindow::Place::MapCorner, 0);
    HGM_CHECK(unknown.title() == "Star System Unexplored");
    HGM_CHECK(!unknown.planetRect(0).has_value());
}
