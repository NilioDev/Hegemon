// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/fleet_window.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Fleet window: the measured Alkari fleet at its home star") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    Game game;
    Star altair;
    altair.name = "Altair";
    altair.x = 669;
    altair.y = 516;
    Star sal;
    sal.name = "Sal";
    sal.x = 607;
    sal.y = 437;
    game.galaxy.stars = {altair, sal};
    Player alkari;
    alkari.race = 0;
    ShipDesign scout;
    scout.image = 5;
    alkari.designs = {scout};
    game.players = {alkari};
    game.ships = {{0, ShipKind::Scout, 0}, {0, ShipKind::Scout, 0}, {0, ShipKind::ColonyShip, 0}};
    for (Ship& s : game.ships) {
        s.speed = 2;
    }
    FleetWindow window(data, game, fleets(game).at(0), 0);
    HGM_CHECK(window.status() == "Orbiting Altair");
    HGM_CHECK(window.chosen().size() == 3);
    // Measured places: the boxes from (37, 60) 58 apart, All at (39, 117),
    // Close at (24, 150).
    HGM_CHECK(window.shipBox(1).x == 95 && window.shipBox(1).y == 60);
    HGM_CHECK(window.allButton().x == 39 && window.allButton().y == 117);
    HGM_CHECK(window.closeButton().x == 24 && window.closeButton().y == 150);
    // A box changes whether its ship goes; All takes them all again.
    const Rect box = window.shipBox(2);
    window.press(box.centerX(), box.centerY());
    HGM_CHECK(window.release(box.centerX(), box.centerY()) == FleetWindow::Action::None);
    HGM_CHECK(window.chosen().size() == 2);
    const Rect all = window.allButton();
    window.press(all.centerX(), all.centerY());
    window.release(all.centerX(), all.centerY());
    HGM_CHECK(window.chosen().size() == 3);
    const Rect close = window.closeButton();
    window.press(close.centerX(), close.centerY());
    HGM_CHECK(window.release(close.centerX(), close.centerY()) == FleetWindow::Action::Close);

    Canvas canvas(640, 480, 0);
    Palette palette;
    window.draw(canvas, palette);
    // The title in green, the chosen boxes' blue behind the ships.
    int green = 0;
    for (int y = 36; y < 50; ++y) {
        for (int x = 80; x < 162; ++x) {
            green += canvas.at(x, y) == 115 ? 1 : 0;
        }
    }
    HGM_CHECK(green > 100);

    // Sent off: "ETA 2 turns" for 100 map units at 2 parsecs.
    for (Ship& s : game.ships) {
        s.destination = 1;
    }
    window.update(game, fleets(game).at(0));
    HGM_CHECK(window.status() == "ETA 2 turns");
    HGM_CHECK(window.chosen().empty());
}
