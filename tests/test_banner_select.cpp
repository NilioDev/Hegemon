// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/banner_select.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using Colour = hegemon::BannerSelectScreen::Colour;

HGM_TEST("Text lists: one record of texts ending at zeros") {
    const std::vector<std::uint8_t> entry = {1, 0, 7, 0, 'a', 'b', 0, 0, 'c', 0, 0};
    const auto list = hegemon::parseStringList(entry);
    HGM_CHECK(list.size() == 3);
    HGM_CHECK(list[0] == "ab" && list[1].empty() && list[2] == "c");
    const std::vector<std::uint8_t> cut = {1, 0, 9, 0, 'a', 0};
    bool threw = false;
    try {
        (void)hegemon::parseStringList(cut);
    } catch (const hegemon::FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);
}

HGM_TEST("Banner colours: banners where the original has them, taken on release") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    hegemon::BannerSelectScreen screen(data, 0);
    const auto red = screen.bannerRect(0);
    HGM_CHECK(red.x == 96 && red.y == 145 && red.w == 84 && red.h == 94);
    HGM_CHECK(screen.bannerRect(7).x == 467 && screen.bannerRect(7).y == 281);
    HGM_CHECK(!screen.bannerAt(95, 190));
    HGM_CHECK(screen.click(96, 145) == Colour::Red);
    HGM_CHECK(screen.click(500, 340) == Colour::Orange);
    // Pressed anywhere, let go over a banner: taken. Let go elsewhere: not.
    screen.press(600, 450);
    HGM_CHECK(screen.release(140, 190) == Colour::Red);
    screen.press(140, 190);
    HGM_CHECK(!screen.release(600, 450));
    HGM_CHECK(!screen.release(140, 190));

    hegemon::Canvas canvas(640, 480);
    hegemon::Palette palette;
    screen.draw(canvas, palette);
    const hegemon::GeneratingScreen generating(data);
    HGM_CHECK(generating.text() == "Generating Universe ...");
    generating.draw(canvas, palette);
    // Measured: the G's first column, in colour 124, at (229, 215).
    HGM_CHECK(canvas.at(229, 215) == 124);
}
