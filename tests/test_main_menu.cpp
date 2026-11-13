// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/main_menu.hpp"

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

    // The gamepad skips Load, which is greyed out.
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

HGM_TEST("Main menu: the version text at the bottom right") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(menuFolder(game, {}));
    const hegemon::MainMenu menu(data);
    hegemon::Canvas canvas(640, 480);
    hegemon::Palette palette;
    menu.draw(canvas, palette);
    // The V's two top corners, in shade 2; the rest of its first row is the
    // background.
    HGM_CHECK(canvas.at(492, 433) == 215);
    HGM_CHECK(canvas.at(496, 433) == 215);
    int lit = 0;
    for (int y = 433; y < 440; ++y) {
        for (int x = 492; x < 560; ++x) {
            lit += canvas.at(x, y) == 215 || canvas.at(x, y) == 208 ? 1 : 0;
        }
    }
    HGM_CHECK(lit > 50);
    fs::remove_all(fs::temp_directory_path() / "hegemon_test_main_menu");
}
