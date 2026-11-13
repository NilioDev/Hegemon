// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/new_game.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using Item = hegemon::NewGameScreen::Item;
using Result = hegemon::NewGameScreen::Result;
using Settings = hegemon::NewGameSettings;

namespace {

// The middle of an item's picture, or of a switch or button.
std::pair<int, int> centre(const hegemon::NewGameScreen& screen, Item item) {
    const auto r = screen.itemRect(item);
    return {r.x + r.w / 2, r.y + (item <= Item::TechLevel ? 30 : r.h / 2)};
}

void clickOn(hegemon::NewGameScreen& screen, Item item) {
    const auto [x, y] = centre(screen, item);
    screen.click(x, y);
}

// The first column in rows [top, bottom) with a pixel in colour 121 or 123,
// the two greens of the texts.
int firstTextColumn(const hegemon::Canvas& c, int left, int right, int top, int bottom) {
    for (int x = left; x < right; ++x) {
        for (int y = top; y < bottom; ++y) {
            if (c.at(x, y) == 121 || c.at(x, y) == 123) {
                return x;
            }
        }
    }
    return -1;
}

bool samePicture(const hegemon::Canvas& c, const hegemon::Image& picture, int x0, int y0) {
    hegemon::Canvas alone(640, 480);
    picture.draw(0, alone, x0, y0);
    for (int y = 0; y < picture.height(); ++y) {
        for (int x = 0; x < picture.width(); ++x) {
            if (alone.at(x0 + x, y0 + y) != 0 && alone.at(x0 + x, y0 + y) != c.at(x0 + x, y0 + y)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

HGM_TEST("New Game: the fan patch's first settings, and what a click steps to") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    if (data.config() == nullptr) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    hegemon::NewGameScreen screen(data, Settings{});
    HGM_CHECK(screen.valueText(Item::Difficulty) == "Tutor");
    HGM_CHECK(screen.valueText(Item::GalaxySize) == "Medium");
    HGM_CHECK(screen.valueText(Item::GalaxyAge) == "Average");
    HGM_CHECK(screen.valueText(Item::Players) == "5 Players");
    HGM_CHECK(screen.valueText(Item::TechLevel) == "Average");
    HGM_CHECK(screen.settings().tacticalCombat && screen.settings().randomEvents && screen.settings().antaransAttack);

    // The orders measured on the original; each wraps around.
    std::string sizes;
    for (int i = 0; i < 5; ++i) {
        clickOn(screen, Item::GalaxySize);
        sizes += std::string(screen.valueText(Item::GalaxySize)) + ",";
    }
    HGM_CHECK(sizes == "Large,Cluster,Huge,Small,Medium,");
    std::string ages;
    for (int i = 0; i < 3; ++i) {
        clickOn(screen, Item::GalaxyAge);
        ages += std::string(screen.valueText(Item::GalaxyAge)) + ",";
    }
    HGM_CHECK(ages == "Organic Rich,Mineral Rich,Average,");
    std::string players;
    for (int i = 0; i < 8; ++i) {
        clickOn(screen, Item::Players);
        players += std::string(screen.valueText(Item::Players)) + ",";
    }
    HGM_CHECK(players == "6 Players,7 Players,8 Players,Random,2 Players,3 Players,4 Players,5 Players,");
    std::string techs;
    for (int i = 0; i < 4; ++i) {
        clickOn(screen, Item::TechLevel);
        techs += std::string(screen.valueText(Item::TechLevel)) + ",";
    }
    HGM_CHECK(techs == "Post-warp,Advanced,Pre-warp,Average,");
    std::string difficulties;
    for (int i = 0; i < 5; ++i) {
        clickOn(screen, Item::Difficulty);
        difficulties += std::string(screen.valueText(Item::Difficulty)) + ",";
    }
    HGM_CHECK(difficulties == "Easy,Average,Hard,Impossible,Tutor,");
    HGM_CHECK(screen.settings() == Settings{});

    // The box under a picture counts too, the gap between them not.
    screen.click(325, 200);
    HGM_CHECK(screen.valueText(Item::GalaxySize) == "Medium");
    screen.click(325, 218);
    HGM_CHECK(screen.valueText(Item::GalaxySize) == "Large");
    // Let go somewhere else: nothing.
    screen.press(325, 150);
    HGM_CHECK(!screen.release(325, 60));
    HGM_CHECK(screen.valueText(Item::GalaxySize) == "Large");
}

HGM_TEST("New Game: switches flip on press, buttons are taken on release") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    if (data.config() == nullptr) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    hegemon::NewGameScreen screen(data, Settings{});
    HGM_CHECK(screen.itemAt(410, 280) == Item::TacticalCombat);
    screen.press(410, 280);
    HGM_CHECK(!screen.settings().tacticalCombat);
    HGM_CHECK(!screen.release(410, 280));
    HGM_CHECK(!screen.settings().tacticalCombat);
    screen.click(410, 315);
    HGM_CHECK(!screen.settings().randomEvents);
    screen.click(410, 350);
    HGM_CHECK(!screen.settings().antaransAttack);
    screen.click(410, 350);
    HGM_CHECK(screen.settings().antaransAttack);

    const auto [ax, ay] = centre(screen, Item::Accept);
    const auto [cx, cy] = centre(screen, Item::Cancel);
    HGM_CHECK(screen.click(ax, ay) == Result::Accept);
    HGM_CHECK(screen.click(cx, cy) == Result::Cancel);
    // Down on Accept, let go on Cancel: Cancel.
    screen.press(ax, ay);
    screen.pointerMoved(cx, cy);
    HGM_CHECK(screen.release(cx, cy) == Result::Cancel);
    HGM_CHECK(hegemon::NewGameScreen::escape() == Result::Cancel);

    // Held down over Accept, it looks pressed; nothing else changes.
    hegemon::Palette palette;
    hegemon::Canvas up(640, 480);
    screen.pointerMoved(ax, ay);
    screen.draw(up, palette);
    screen.press(ax, ay);
    hegemon::Canvas down(640, 480);
    screen.draw(down, palette);
    const auto accept = screen.itemRect(Item::Accept);
    int changed = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if (up.at(x, y) != down.at(x, y)) {
                ++changed;
                HGM_CHECK(accept.contains(x, y));
            }
        }
    }
    HGM_CHECK(changed > 100);
    // Switched off, only the lamp changes.
    hegemon::Canvas off(640, 480);
    screen.release(0, 0);
    screen.press(410, 280);
    screen.draw(off, palette);
    hegemon::Canvas on(640, 480);
    screen.press(410, 280);
    screen.draw(on, palette);
    changed = 0;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            if (on.at(x, y) != off.at(x, y)) {
                ++changed;
                HGM_CHECK(x >= 395 && x < 430 && y >= 264 && y < 296);
            }
        }
    }
    HGM_CHECK(changed > 50);
}

HGM_TEST("New Game: pictures and texts where the original draws them") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    hegemon::GameData data(game);
    if (data.config() == nullptr) {
        HGM_SKIP("fan patch 1.51 not in gamedata/");
    }
    const auto& archive = data.archive("NEWGAME.LBX");
    const auto picture = [&](std::size_t entry) { return hegemon::Image::parse(archive.entry(entry)); };
    hegemon::Palette palette;
    {
        hegemon::NewGameScreen screen(data, Settings{});
        hegemon::Canvas c(640, 480);
        screen.draw(c, palette);
        HGM_CHECK(samePicture(c, picture(4), 136, 124));  // Tutor
        HGM_CHECK(samePicture(c, picture(10), 291, 124)); // Medium
        HGM_CHECK(samePicture(c, picture(2), 446, 124));  // Average age
        HGM_CHECK(samePicture(c, picture(17), 136, 264)); // 5 players
        HGM_CHECK(samePicture(c, picture(23), 291, 264)); // Average tech
        // Where the texts start, measured on the original.
        HGM_CHECK(firstTextColumn(c, 116, 222, 209, 229) == 155);
        HGM_CHECK(firstTextColumn(c, 272, 378, 209, 229) == 305);
        HGM_CHECK(firstTextColumn(c, 428, 534, 209, 229) == 459);
        HGM_CHECK(firstTextColumn(c, 116, 222, 354, 374) == 144);
        HGM_CHECK(firstTextColumn(c, 272, 378, 354, 374) == 303);
        // The top of Tutor's T, in the bright green.
        HGM_CHECK(c.at(155, 214) == 123);
        HGM_CHECK(c.at(159, 214) == 123);
        HGM_CHECK(c.at(157, 222) == 123);
        HGM_CHECK(c.at(155, 213) != 123);
    }
    {
        Settings s;
        s.galaxySize = Settings::GalaxySize::Cluster;
        s.players = Settings::kRandomPlayers;
        s.techLevel = Settings::TechLevel::PostWarp;
        hegemon::NewGameScreen screen(data, s);
        hegemon::Canvas c(640, 480);
        screen.draw(c, palette);
        HGM_CHECK(samePicture(c, picture(12), 291, 124));
        HGM_CHECK(samePicture(c, picture(21), 136, 264));
        HGM_CHECK(samePicture(c, picture(24), 291, 264));
        s.galaxySize = Settings::GalaxySize::Huge;
        s.techLevel = Settings::TechLevel::Advanced;
        hegemon::NewGameScreen other(data, s);
        other.draw(c, palette);
        HGM_CHECK(samePicture(c, picture(13), 291, 124));
        HGM_CHECK(samePicture(c, picture(25), 291, 264));
    }
}
