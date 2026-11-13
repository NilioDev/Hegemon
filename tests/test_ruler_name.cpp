// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/ruler_name.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

HGM_TEST("Ruler name: the names offered, and what a key may add") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    const hegemon::RulerNames names(data);
    HGM_CHECK(names.name(0, 7) == "Karaaw Hrik"); // Alkari
    HGM_CHECK(names.name(1, 0) == "Trebakka");    // Bulrathi

    // Read off the original: "Karaaw Hrik" takes two more x's, seven W's
    // fit, nineteen i's.
    hegemon::RulerNameDialog dialog(data, 0, names.name(0, 7));
    HGM_CHECK(dialog.type('x'));
    HGM_CHECK(dialog.type('x'));
    HGM_CHECK(!dialog.type('x'));
    HGM_CHECK(dialog.name() == "Karaaw Hrikxx");
    for (int i = 0; i < 20; ++i) {
        dialog.backspace();
    }
    HGM_CHECK(dialog.name().empty());
    int typed = 0;
    while (dialog.type('W')) {
        ++typed;
    }
    HGM_CHECK(typed == 7);
    hegemon::RulerNameDialog narrow(data, 0, "");
    typed = 0;
    while (narrow.type('i')) {
        ++typed;
    }
    HGM_CHECK(typed == 19);
    HGM_CHECK(!narrow.type('\r'));

    // Accept takes the name when the mouse button is let go over it.
    HGM_CHECK(dialog.click(322, 240));
    HGM_CHECK(!dialog.release(322, 240));
    HGM_CHECK(!dialog.click(100, 100));
    dialog.press(100, 100);
    HGM_CHECK(dialog.release(273, 225));
}

HGM_TEST("Ruler name: drawn in the nearest colours of the race's screen") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    hegemon::Canvas canvas(640, 480);
    hegemon::Palette palette;
    // Measured on the original: the same name is lighter with the Alkari's
    // colours than with the Bulrathi's.
    hegemon::RulerNameDialog alkari(data, 0, "Karaaw Hrik");
    alkari.draw(canvas, palette);
    HGM_CHECK((palette.colors[canvas.at(281, 190)] == hegemon::Color{182, 182, 182}));
    hegemon::RulerNameDialog bulrathi(data, 1, "Karaaw Hrik");
    bulrathi.draw(canvas, palette);
    HGM_CHECK((palette.colors[canvas.at(281, 190)] == hegemon::Color{166, 166, 166}));

    // The cursor after the name blinks with the PC timer.
    HGM_CHECK(canvas.at(310, 196) == canvas.at(281, 190));
    bulrathi.tick(55);
    HGM_CHECK(!bulrathi.cursorShown());
    bulrathi.draw(canvas, palette);
    HGM_CHECK(canvas.at(310, 196) != canvas.at(281, 190));
}
