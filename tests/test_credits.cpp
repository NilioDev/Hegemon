// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/credits.hpp"

#include <filesystem>
#include <set>

namespace {

constexpr std::uint8_t kEmpty = 255;

// The colours used in rows [top, bottom) of the canvas.
std::set<int> coloursIn(const hegemon::Canvas& canvas, int top, int bottom) {
    std::set<int> colours;
    for (int y = top; y < bottom; ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.at(x, y) != kEmpty) {
                colours.insert(canvas.at(x, y));
            }
        }
    }
    return colours;
}

} // namespace

HGM_TEST("Credits: two pixels up every third timer tick, starting again after the last line") {
    const std::filesystem::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    hegemon::Credits credits(data);
    HGM_CHECK(credits.offset() == 0);
    HGM_CHECK(credits.lineTop(0) == 456);
    HGM_CHECK(credits.lineTop(1) == 470);
    // One second is 18 ticks of the PC timer: 6 steps.
    credits.tick(1000);
    HGM_CHECK(credits.offset() == 12);
    // Time adds up across short frames.
    for (int i = 0; i < 60; ++i) {
        credits.tick(16);
    }
    HGM_CHECK(credits.offset() == 22);

    // The last of the 57 lines at the top of the window: one step later the
    // credits start again from below.
    hegemon::Credits fresh(data);
    fresh.setOffset(456 + 14 * 56 - 180);
    HGM_CHECK(fresh.lineTop(56) == 180);
    fresh.tick(165);
    HGM_CHECK(fresh.offset() == 0);
}

HGM_TEST("Credits: drawn only inside their window, fading at the top and the bottom") {
    const std::filesystem::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    hegemon::Credits credits(data);
    // Line 11 at the top (top 172), line 18 in the middle (top 270) and line
    // 28 at the bottom (top 410); none of them is empty.
    credits.setOffset(438);
    HGM_CHECK(credits.lineTop(11) == 172);
    HGM_CHECK(credits.lineTop(18) == 270);
    HGM_CHECK(credits.lineTop(28) == 410);
    hegemon::Canvas canvas(640, 480, kEmpty);
    credits.draw(canvas);

    HGM_CHECK(coloursIn(canvas, 0, 179).empty());
    HGM_CHECK(coloursIn(canvas, 423, 480).empty());
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 66; ++x) {
            HGM_CHECK(canvas.at(x, y) == kEmpty);
        }
    }
    HGM_CHECK(coloursIn(canvas, 410, 423) == std::set<int>{0, 211, 219});
    HGM_CHECK(coloursIn(canvas, 179, 186) == std::set<int>{0, 212, 213});
    HGM_CHECK(coloursIn(canvas, 270, 284) == std::set<int>{0, 213, 220});
}
