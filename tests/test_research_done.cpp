// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/research_done.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

HGM_TEST("Research done: the texts, the animations and the colours") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const TechTree tree(data.parameters());
    // Measured: Reinforced Hull, learnt in Advanced Engineering.
    constexpr int kReinforcedHull = 56;
    ResearchDoneScreen screen(data, tree, 0, tree.techField(kReinforcedHull), kReinforcedHull);
    HGM_CHECK(screen.title() == "Reinforced Hull");
    HGM_CHECK(screen.description().size() == 5);
    HGM_CHECK(!screen.description().empty() && screen.description()[0] == "Triples the structural integrity");
    HGM_CHECK(screen.message().size() == 2);
    HGM_CHECK(screen.message().size() == 2 && screen.message()[1] == "in Advanced Engineering!");

    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    // The name's letters in green with dark edges; the message's with a
    // black border.
    HGM_CHECK((palette.colors[246] == Color{8, 174, 0}) && (palette.colors[245] == Color{8, 150, 0}));
    HGM_CHECK(canvas.at(151, 420) == 245 && canvas.at(153, 420) == 0);

    // A frame a tick, about 18 a second; the panel starts again at frame 5.
    screen.tick(1000);
    const auto f = screen.frames();
    HGM_CHECK(f[0] == 18u && f[1] == 5u && f[2] == 18u && f[3] == 8u);

    bool threw = false;
    try {
        ResearchDoneScreen none(data, tree, 0, 1, 100000);
    } catch (const FormatError&) {
        threw = true;
    } catch (const std::exception&) {
        threw = true;
    }
    HGM_CHECK(threw);
}
