// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/main_menu.hpp"
#include "hegemon/ui/pointer.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using Choice = hegemon::MainMenu::Choice;

namespace {

// A folder with the menu's archives from the player's files and the given
// savegames (empty files are enough for the menu).
fs::path menuFolder(const fs::path& game, std::initializer_list<const char*> saves) {
    const fs::path folder = fs::temp_directory_path() / "hegemon_test_main_menu";
    fs::remove_all(folder);
    fs::create_directories(folder);
    for (const char* name : {"MAINMENU.LBX", "FONTS.LBX", "CREDITS.LBX"}) {
        fs::copy_file(*hegemon::findFileNoCase(game, name), folder / name);
    }
    for (const char* save : saves) {
        std::ofstream(folder / save) << "x";
    }
    return folder;
}

} // namespace

HGM_TEST("Main menu: Continue needs the autosave, Load a game in slots 1 to 9") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    {
        hegemon::GameData data(menuFolder(game, {}));
        const hegemon::MainMenu menu(data);
        HGM_CHECK(!menu.enabled(Choice::Continue));
        HGM_CHECK(!menu.enabled(Choice::Load));
        HGM_CHECK(menu.enabled(Choice::NewGame));
        HGM_CHECK(menu.enabled(Choice::Quit));
    }
    {
        hegemon::GameData data(menuFolder(game, {"SAVE10.GAM"}));
        const hegemon::MainMenu menu(data);
        HGM_CHECK(menu.enabled(Choice::Continue));
        HGM_CHECK(!menu.enabled(Choice::Load));
    }
    {
        hegemon::GameData data(menuFolder(game, {"save3.gam"}));
        const hegemon::MainMenu menu(data);
        HGM_CHECK(!menu.enabled(Choice::Continue));
        HGM_CHECK(menu.enabled(Choice::Load));
    }
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}

HGM_TEST("Main menu: buttons where the original has them; pointer, clicks and gamepad") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(menuFolder(game, {"SAVE10.GAM"}));
    hegemon::MainMenu menu(data);
    menu.skipOpening();
    const auto quit = menu.buttonRect(5);
    HGM_CHECK(quit.x == 415);
    HGM_CHECK(quit.y == 285);
    HGM_CHECK(quit.w == 153);
    HGM_CHECK(quit.h == 22);

    // Over a button that can be used: highlighted. Over a greyed-out one or
    // beside the panel: nothing.
    menu.pointerMoved(450, 250);
    HGM_CHECK(menu.highlighted() == Choice::Multiplayer);
    menu.pointerMoved(450, 200);
    HGM_CHECK(!menu.highlighted());
    HGM_CHECK(!menu.click(450, 200));
    HGM_CHECK(menu.click(500, 220) == Choice::NewGame);
    HGM_CHECK(!menu.click(100, 100));

    // A button is taken when the mouse button is let go over it, wherever
    // it went down; the original's letter keys take it too.
    menu.press(500, 220);
    HGM_CHECK(!menu.release(450, 200)); // over greyed-out Load: nothing
    menu.press(500, 220);
    HGM_CHECK(menu.release(450, 290) == Choice::Quit);
    HGM_CHECK(!menu.release(450, 290)); // no second release without a press
    menu.press(100, 100);
    HGM_CHECK(menu.release(450, 250) == Choice::Multiplayer);
    HGM_CHECK(menu.hotkey('N') == Choice::NewGame);
    HGM_CHECK(menu.hotkey('c') == Choice::Continue);
    HGM_CHECK(!menu.hotkey('l')); // Load is greyed out
    HGM_CHECK(menu.hotkey('q') == Choice::Quit);
    HGM_CHECK(!menu.hotkey('x'));

    // The gamepad skips Load, which is greyed out.
    menu.pointerMoved(0, 0);
    menu.moveHighlight(1);
    HGM_CHECK(menu.highlighted() == Choice::Continue);
    menu.moveHighlight(1);
    HGM_CHECK(menu.highlighted() == Choice::NewGame);
    menu.moveHighlight(-1);
    HGM_CHECK(menu.highlighted() == Choice::Continue);
    menu.moveHighlight(-1);
    HGM_CHECK(menu.activate() == Choice::Quit);

    // The highlighted button is drawn in its bright state, the others not.
    hegemon::Canvas plain(640, 480);
    hegemon::Canvas lit(640, 480);
    hegemon::Palette palette;
    menu.pointerMoved(0, 0);
    menu.draw(plain, palette);
    menu.pointerMoved(450, 250);
    menu.draw(lit, palette);
    int changed = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if (plain.at(x, y) != lit.at(x, y)) {
                ++changed;
                HGM_CHECK(menu.buttonRect(3).contains(x, y));
            }
        }
    }
    HGM_CHECK(changed > 100);
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}

HGM_TEST("Main menu: version and mod name where the fan patch's settings put them") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    const fs::path folder = menuFolder(game, {});
    std::ofstream(folder / "ORION2.CFG") << "format version = \"Version $VERSION$\" 1 r 541 433;\n"
                                         << "format mod_name = \"\" 1 r 541 419;\n";
    hegemon::GameData data(folder);
    hegemon::MainMenu menu(data);
    menu.skipOpening();
    HGM_CHECK(menu.versionLabel().text == "Version 1.51");
    HGM_CHECK(menu.modNameLabel().text.empty());
    hegemon::Canvas canvas(640, 480);
    hegemon::Palette palette;
    menu.draw(canvas, palette);
    // Right-aligned on x = 541: the V's two top corners, in shade 2.
    HGM_CHECK(canvas.at(492, 433) == 215);
    HGM_CHECK(canvas.at(496, 433) == 215);
    int lit = 0;
    for (int y = 433; y < 440; ++y) {
        for (int x = 492; x < 560; ++x) {
            lit += canvas.at(x, y) == 215 || canvas.at(x, y) == 208 ? 1 : 0;
        }
    }
    HGM_CHECK(lit > 50);
    fs::remove_all(folder);
}

HGM_TEST("Main menu: the fan patch's defaults when its settings don't say") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    const fs::path folder = menuFolder(game, {});
    std::ofstream(folder / "ORION2.CFG") << "format version y = 420;\n";
    hegemon::GameData data(folder);
    const hegemon::MainMenu menu(data);
    HGM_CHECK(menu.versionLabel().text == "Version: 1.51");
    HGM_CHECK(menu.versionLabel().font == 2);
    HGM_CHECK(menu.versionLabel().align == 'r');
    HGM_CHECK(menu.versionLabel().x == 590);
    HGM_CHECK(menu.versionLabel().y == 420);
    HGM_CHECK(menu.modNameLabel().text == "Config: ");
    fs::remove_all(folder);
}

HGM_TEST("Main menu: the opening animation, one frame a tick, then the buttons") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(menuFolder(game, {"SAVE10.GAM"}));
    hegemon::MainMenu menu(data);
    HGM_CHECK(menu.opening());
    hegemon::Palette palette;
    const auto frame = [&] {
        hegemon::Canvas canvas(640, 480);
        menu.draw(canvas, palette);
        return canvas;
    };
    // No highlight and no choice while it opens.
    menu.pointerMoved(450, 250);
    HGM_CHECK(!menu.highlighted());
    const auto first = frame();
    // 49 ticks of the PC timer are a little under 2.7 seconds.
    menu.tick(2600);
    HGM_CHECK(menu.opening());
    const auto later = frame();
    int changed = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            changed += first.at(x, y) != later.at(x, y) ? 1 : 0;
        }
    }
    HGM_CHECK(changed > 10000);
    menu.tick(200);
    HGM_CHECK(!menu.opening());
    menu.pointerMoved(450, 250);
    HGM_CHECK(menu.highlighted() == Choice::Multiplayer);

    // A click while it opens only skips to the end.
    hegemon::MainMenu skipped(data);
    HGM_CHECK(!skipped.click(450, 250));
    HGM_CHECK(!skipped.opening());
    HGM_CHECK(skipped.click(450, 250) == Choice::Multiplayer);
    // The screen it ends on is the same either way.
    hegemon::Canvas other(640, 480);
    menu.pointerMoved(0, 0);
    skipped.pointerMoved(0, 0);
    menu.draw(other, palette);
    hegemon::Canvas mine(640, 480);
    skipped.draw(mine, palette);
    bool same = true;
    for (int y = 0; y < 180; ++y) {
        for (int x = 0; x < 640; ++x) {
            same = same && mine.at(x, y) == other.at(x, y);
        }
    }
    HGM_CHECK(same);
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}

HGM_TEST("Main menu: back from another screen, without the opening and with the credits from the start") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(menuFolder(game, {}));
    hegemon::MainMenu menu(data);
    menu.tick(20000);
    HGM_CHECK(menu.credits().offset() > 0);
    menu.reopen();
    HGM_CHECK(!menu.opening());
    HGM_CHECK(menu.credits().offset() == 0);
    // A press from the other screen does not carry over.
    HGM_CHECK(!menu.release(500, 220));
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}

HGM_TEST("Pointer: the original's, where the mouse is") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(menuFolder(game, {}));
    hegemon::Pointer pointer(data);
    HGM_CHECK(pointer.x() == 320);
    HGM_CHECK(pointer.y() == 240);
    hegemon::Canvas without(640, 480);
    pointer.hide();
    pointer.draw(without);
    HGM_CHECK(!pointer.visible());
    pointer.move(100, 60);
    HGM_CHECK(pointer.visible());
    hegemon::Canvas with(640, 480);
    pointer.draw(with);
    int changed = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if (with.at(x, y) != without.at(x, y)) {
                ++changed;
                HGM_CHECK(x >= 100 && x < 124 && y >= 60 && y < 84);
            }
        }
    }
    HGM_CHECK(changed > 100);
    // Its tip is the hot spot.
    HGM_CHECK(with.at(100, 60) != without.at(100, 60));
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}
