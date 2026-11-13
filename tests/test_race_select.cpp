// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/font.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/race_select.hpp"

#include <filesystem>

namespace fs = std::filesystem;

HGM_TEST("Race selection: buttons where the original has them; the last one pointed at counts") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    if (data.config() == nullptr) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    const hegemon::Races races(data);
    hegemon::RaceSelectScreen screen(data, races, 412, 400); // over Klackons
    HGM_CHECK(screen.highlighted() == 6);
    const auto alkari = screen.buttonRect(0);
    HGM_CHECK(alkari.x == 351 && alkari.y == 90 && alkari.w == 123 && alkari.h == 45);
    const auto custom = screen.buttonRect(hegemon::RaceSelectScreen::kCustom);
    HGM_CHECK(custom.x == 477 && custom.y == 378);

    // Pointing at a race shows it; pointing elsewhere keeps it.
    screen.pointerMoved(538, 112);
    HGM_CHECK(screen.highlighted() == 7); // Meklars
    screen.pointerMoved(100, 100);
    HGM_CHECK(screen.highlighted() == 7);
    HGM_CHECK(screen.descriptionLines().size() == 1);
    HGM_CHECK(screen.descriptionLines()[0] == "Industrial Production: +2, Cybernetic, Dictatorship");
    // Sakkra's text takes three lines, broken where the original breaks it.
    screen.pointerMoved(538, 256);
    HGM_CHECK(screen.highlighted() == 10);
    HGM_CHECK(screen.descriptionLines().size() == 3);
    HGM_CHECK(screen.descriptionLines()[2] == "Feudal");
    // Custom has no text.
    screen.pointerMoved(538, 400);
    HGM_CHECK(screen.descriptionLines().empty());

    // A race is taken when the mouse button is let go over it.
    screen.press(412, 112);
    HGM_CHECK(screen.release(412, 160) == std::size_t{1}); // Bulrathi
    HGM_CHECK(!screen.release(412, 160));
    HGM_CHECK(!screen.click(100, 100));
    HGM_CHECK(screen.click(538, 400) == hegemon::RaceSelectScreen::kCustom);

    // Hegemon's gamepad steps through the buttons and wraps around.
    screen.moveHighlight(1);
    HGM_CHECK(screen.highlighted() == 0);
    screen.moveHighlight(-1);
    HGM_CHECK(screen.highlighted() == hegemon::RaceSelectScreen::kCustom);
    screen.moveHighlight(-7);
    HGM_CHECK(screen.highlighted() == 6);
}

HGM_TEST("Race selection: the text in the original's place and colours, the lit button") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    if (data.config() == nullptr) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    const hegemon::Races races(data);
    hegemon::RaceSelectScreen screen(data, races, 412, 112); // Alkari
    hegemon::Palette palette;
    hegemon::Canvas alkari(640, 480);
    screen.draw(alkari, palette);
    // "Ship Defense: ...": the S starts in shade 1 at (50, 391), shade 2 right
    // of it, with the black shadow below.
    HGM_CHECK(alkari.at(50, 391) == 120);
    HGM_CHECK(alkari.at(51, 391) == 124);
    HGM_CHECK(alkari.at(51, 392) == 0);
    int lit = 0;
    for (int y = 391; y < 420; ++y) {
        for (int x = 48; x < 350; ++x) {
            lit += alkari.at(x, y) == 124 ? 1 : 0;
        }
    }
    HGM_CHECK(lit > 300);

    // Pointing at Bulrathi changes the portrait, the text and the lit
    // buttons, nothing else.
    screen.pointerMoved(412, 160);
    hegemon::Canvas bulrathi(640, 480);
    screen.draw(bulrathi, palette);
    const auto inButtons = [&](int x, int y) {
        return screen.buttonRect(0).contains(x, y) || screen.buttonRect(1).contains(x, y);
    };
    bool elsewhere = false;
    int portrait = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if (alkari.at(x, y) == bulrathi.at(x, y)) {
                continue;
            }
            const bool inPortrait = x >= 54 && x < 344 && y >= 63 && y < 385;
            const bool inText = x >= 48 && x < 350 && y >= 388 && y < 430;
            portrait += inPortrait ? 1 : 0;
            elsewhere = elsewhere || !(inPortrait || inText || inButtons(x, y));
        }
    }
    HGM_CHECK(!elsewhere);
    HGM_CHECK(portrait > 10000);
}

HGM_TEST("Fonts: words wrapped to a width") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    const auto fonts = hegemon::FontSet::parse(data.archive("FONTS.LBX").entry(0));
    const auto& font = fonts.font(2);
    const auto lines = hegemon::wrapWords(font, "aa bb  cc", font.textWidth("aa bb", 2) - 2, 2);
    HGM_CHECK(lines.size() == 2);
    HGM_CHECK(lines[0] == "aa bb");
    HGM_CHECK(lines[1] == "cc");
    // A word too wide for the line still gets one.
    const auto one = hegemon::wrapWords(font, "Unification", 5, 2);
    HGM_CHECK(one.size() == 1 && one[0] == "Unification");
    HGM_CHECK(hegemon::wrapWords(font, "", 100, 2).empty());
}
