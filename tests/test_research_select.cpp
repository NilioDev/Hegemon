// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/research_select.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Research selection: the boxes, the lit one and the field taken") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const TechTree tree(data.parameters());
    const Knowledge k = tree.averageStart();
    ResearchSelectScreen screen(data, tree, k, 0, 0, 0);
    HGM_CHECK(!screen.highlighted());
    // Measured: the boxes' borders when lit.
    HGM_CHECK(screen.boxRect(0).x == 174 && screen.boxRect(0).y == 48 && screen.boxRect(0).w == 218 &&
              screen.boxRect(0).h == 82);
    HGM_CHECK(screen.boxRect(7).x == 401 && screen.boxRect(7).y == 365);
    for (std::size_t i = 0; i < ResearchSelectScreen::kBoxes; ++i) {
        HGM_CHECK(screen.field(i) == tree.researchable(k, ResearchSelectScreen::kOrder[i]));
    }
    // The pointer on Construction's second application, Reinforced Hull.
    screen.pointerMoved(290, 90);
    HGM_CHECK(screen.highlighted() == 0u && screen.pointedApp() == 1u);
    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    // The border and the arrow in the bright colour, the lit name in the
    // lit green.
    HGM_CHECK(canvas.at(300, 48) == 251 && canvas.at(177, 80) == 251 && canvas.at(184, 90) == 251);
    HGM_CHECK((palette.colors[251] == Color{158, 251, 28}));
    int lit = 0;
    for (int x = 176; x < 300; ++x) {
        for (int y = 51; y < 66; ++y) {
            lit += canvas.at(x, y) == 231 ? 1 : 0;
        }
    }
    HGM_CHECK(lit > 300);
    HGM_CHECK((palette.colors[231] == Color{40, 203, 0}) && (palette.colors[228] == Color{4, 121, 0}));
    // A click takes the field; outside the boxes nothing.
    HGM_CHECK(screen.click(500, 300) == screen.field(5));
    HGM_CHECK(!screen.click(100, 200));
    // The gamepad goes round the boxes.
    screen.moveHighlight(1);
    HGM_CHECK(screen.highlighted() == 0u && screen.choose() == screen.field(0));
    screen.moveHighlight(-1);
    HGM_CHECK(screen.highlighted() == 7u);
    screen.tick(1000);
}
