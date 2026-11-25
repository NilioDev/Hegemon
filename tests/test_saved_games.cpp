// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/savegame.hpp"
#include "hegemon/ui/saved_games.hpp"
#include "hegemon/ui/main_menu.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Load Game: the date and time a slot shows") {
    HGM_CHECK(savedDate(2026, 11, 25) == "Nov 25, 2026");
    HGM_CHECK(savedDate(2027, 1, 3) == "Jan 3, 2027");
    HGM_CHECK(savedTime(2, 34) == "02:34");
    HGM_CHECK(savedTime(23, 5) == "23:05");
}

HGM_TEST("Load Game: a saved game's name and stardate") {
    std::vector<std::uint8_t> b(kSavegameSize, 0);
    b[0] = 0xe0;
    const char name[] = "(Auto Save)";
    for (std::size_t i = 0; i + 1 < sizeof name; ++i) {
        b[4 + i] = static_cast<std::uint8_t>(name[i]);
    }
    b[41] = 35001 & 0xff;
    b[42] = 35001 >> 8;
    const SavegameSummary s = readSavegameSummary(b);
    HGM_CHECK(s.name == "(Auto Save)" && s.stardate == 35001);
    b.pop_back();
    bool refused = false;
    try {
        readSavegameSummary(b);
    } catch (const FormatError&) {
        refused = true;
    }
    HGM_CHECK(refused);
}

HGM_TEST("Load Game: the measured window over the main menu") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    SavedGamesWindow::Slots slots;
    slots[0] = SaveSlot{"First Save", 35001, "Nov 25, 2026", "02:34"};
    slots[9] = SaveSlot{"(Auto Save)", 35001, "Nov 25, 2026", "02:24"};
    using Kind = SavedGamesWindow::Result::Kind;
    using W = SavedGamesWindow;

    // Measured: one click on a slot loads it, an empty one too (to be told
    // it isn't there); Load needs a slot used before.
    W window(data, W::Kind::Load, W::Over::MainMenu, slots, std::nullopt);
    const Rect first = window.slotRect(0), third = window.slotRect(2);
    HGM_CHECK(first.x == 202 && first.y == 71 && third.y == 133);
    auto r = window.press(first.centerX(), first.centerY());
    HGM_CHECK(r.kind == Kind::Load && r.slot == 0);
    r = window.press(third.centerX(), third.centerY());
    HGM_CHECK(r.kind == Kind::Load && r.slot == 2);
    const Rect load = window.okRect(), cancel = window.cancelRect();
    HGM_CHECK(load.x == 217 && load.y == 388 && cancel.x == 351 && cancel.y == 389);
    window.press(load.centerX(), load.centerY());
    HGM_CHECK(window.release(load.centerX(), load.centerY()).kind == Kind::None);
    window.press(cancel.centerX(), cancel.centerY());
    HGM_CHECK(window.release(cancel.centerX(), cancel.centerY()).kind == Kind::Cancel);
    window.press(cancel.centerX(), cancel.centerY());
    HGM_CHECK(window.release(0, 0).kind == Kind::None);

    W again(data, W::Kind::Load, W::Over::MainMenu, slots, 9);
    again.press(load.centerX(), load.centerY());
    r = again.release(load.centerX(), load.centerY());
    HGM_CHECK(r.kind == Kind::Load && r.slot == 9);

    // Drawn over the menu: the slot used last brighter, 252 for 255.
    MainMenu menu(data);
    menu.skipOpening();
    Canvas canvas(640, 480, 0);
    Palette palette;
    menu.draw(canvas, palette, true);
    again.draw(canvas, palette);
    int normal = 0, bright = 0;
    for (int y = first.y; y < first.y + first.h; ++y) {
        for (int x = first.x; x < first.x + first.w; ++x) {
            normal += canvas.at(x, y) == 255 ? 1 : 0;
        }
    }
    const Rect autosave = again.slotRect(9);
    for (int y = autosave.y; y < autosave.y + autosave.h; ++y) {
        for (int x = autosave.x; x < autosave.x + autosave.w; ++x) {
            bright += canvas.at(x, y) == 252 ? 1 : 0;
        }
    }
    HGM_CHECK(normal > 100 && bright > 100);

    // The warning's text, red, centred on 322.
    WarningBox warning(data, "SAVE3.GAM does not exist");
    warning.advance(440);
    warning.draw(canvas, palette);
    int red = 0;
    for (int y = 232; y < 246; ++y) {
        for (int x = 213; x < 431; ++x) {
            red += canvas.at(x, y) == 137 ? 1 : 0;
        }
    }
    HGM_CHECK(red > 500);
}

HGM_TEST("Save Game: typing a name in a slot over the galaxy map") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    using W = SavedGamesWindow;
    using Kind = W::Result::Kind;
    W::Slots slots;
    slots[1] = SaveSlot{"Second Game Here", 35001, "Nov 25, 2026", "02:34"};
    W window(data, W::Kind::Save, W::Over::Game, slots, std::nullopt);
    // Measured: over the map the window is at (144, 25), Save at (181, 363).
    HGM_CHECK(window.slotRect(0).x == 166 && window.slotRect(0).y == 45);
    HGM_CHECK(window.okRect().x == 181 && window.okRect().y == 363);
    HGM_CHECK(!window.type('x'));
    // An empty slot starts from its words, a used one from its name.
    const Rect fourth = window.slotRect(3), second = window.slotRect(1);
    window.press(fourth.centerX(), fourth.centerY());
    HGM_CHECK(window.editing() == 3 && window.name() == "... empty slot ...");
    for (int k = 0; k < 20; ++k) {
        window.backspace();
    }
    HGM_CHECK(window.type('A') && window.type('b') && window.type('c'));
    auto r = window.enter();
    HGM_CHECK(r.kind == Kind::Save && r.slot == 3 && r.name == "Abc");
    window.press(second.centerX(), second.centerY());
    window.type('X');
    const Rect ok = window.okRect();
    window.press(ok.centerX(), ok.centerY());
    r = window.release(ok.centerX(), ok.centerY());
    HGM_CHECK(r.kind == Kind::Save && r.slot == 1 && r.name == "Second Game HereX");
    // A name stops growing at 200 pixels.
    while (window.type('W')) {
    }
    HGM_CHECK(window.name().size() < 36);

    Canvas canvas(640, 480, 0);
    Palette palette;
    window.draw(canvas, palette);
    GameMenu menu(data);
    menu.draw(canvas);
    const Rect save = GameMenu::buttonRect(GameMenu::Button::Save);
    HGM_CHECK(save.x == 184 && save.y == 68);
    menu.press(save.centerX(), save.centerY());
    HGM_CHECK(menu.release(save.centerX(), save.centerY()) == GameMenu::Button::Save);
    menu.press(save.centerX(), save.centerY());
    HGM_CHECK(!menu.release(0, 0));
}
