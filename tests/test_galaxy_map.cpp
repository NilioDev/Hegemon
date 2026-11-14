// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/ui/galaxy_map.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

Galaxy medium() {
    Galaxy g;
    g.width = 759;
    g.height = 600;
    g.mapSize = 1;
    Star orion;
    orion.name = "Orion";
    orion.x = 365;
    orion.y = 166;
    orion.colour = StarColour::Yellow;
    orion.size = 0;
    g.stars.push_back(orion);
    Star home;
    home.name = "Altair";
    home.x = 76;
    home.y = 133;
    home.colour = StarColour::Orange;
    home.size = 1;
    g.stars.push_back(home);
    g.homes.push_back({0, 1, kNoPlanet});
    return g;
}

} // namespace

HGM_TEST("Galaxy map: where the stars are drawn, and the frame around them") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(game);
    const Galaxy g = medium();
    const GalaxyMap map(data, g, {0});
    // Measured: a medium galaxy is drawn at two thirds, from (21, 21).
    HGM_CHECK(map.screenX(365) == 264 && map.screenY(166) == 131);
    HGM_CHECK(map.screenX(76) == 71 && map.screenY(133) == 109);
    HGM_CHECK(map.screenX(0) == 21 && map.screenX(758) == 526);

    Canvas canvas(640, 480, 99);
    Palette palette;
    map.draw(canvas, palette);
    // The star's middle is drawn, the frame opening's left edge is black.
    HGM_CHECK(canvas.at(264, 131) != 0);
    int black = 0;
    for (int y = GalaxyMap::kView.y; y < GalaxyMap::kView.y + GalaxyMap::kView.h; ++y) {
        black += canvas.at(GalaxyMap::kView.x, y) == 0 ? 1 : 0;
    }
    HGM_CHECK(black > 180); // the rest is the frame
    // Nothing of the screen before shows through the sky.
    Canvas other(640, 480, 7);
    map.draw(other, palette);
    HGM_CHECK(std::ranges::equal(canvas.pixels(), other.pixels()));
    // The home star's name in red under it: the letters' middle under the
    // star's.
    int left = 640, right = -1;
    for (int y = 120; y < 130; ++y) {
        for (int x = 40; x < 100; ++x) {
            if (canvas.at(x, y) == 79) {
                left = std::min(left, x);
                right = std::max(right, x);
            }
        }
    }
    HGM_CHECK(left == 60 && right == 82);
}

HGM_TEST("Galaxy map: the side panel and the fleet at the home star") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(game);
    const Galaxy g = medium();
    GalaxyMap map(data, g, {0});
    Canvas before(640, 480, 99);
    Palette palette;
    map.draw(before, palette);
    EmpireSummary s;
    s.money = 50;
    s.income = 7;
    s.commandPoints = 6;
    s.commandUsed = 3;
    map.setPanel(35000, s);
    map.setFleets({GalaxyMap::Marker{1}});
    Canvas after(640, 480, 99);
    map.draw(after, palette);
    const auto count = [&](int x0, int y0, int x1, int y1, std::uint8_t colour) {
        int n = 0;
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                n += after.at(x, y) == colour && before.at(x, y) != colour ? 1 : 0;
            }
        }
        return n;
    };
    // The texts in light grey: the stardate, "50 BC", "+7 BC", "none".
    HGM_CHECK(count(555, 29, 605, 41, 23) > 30);
    HGM_CHECK(count(555, 96, 605, 120, 23) > 60);
    HGM_CHECK(count(555, 403, 605, 415, 23) > 30);
    // The red fleet marker at the upper right of the size 1 star at (71,
    // 109): its 12 × 10 picture from (78, 96).
    int changed = 0;
    for (int y = 96; y < 106; ++y) {
        for (int x = 78; x < 90; ++x) {
            changed += after.at(x, y) != before.at(x, y) ? 1 : 0;
        }
    }
    HGM_CHECK(changed > 30);

    // With a project: "~11 turns" above "10 RP" in place of "none".
    s.research = 10;
    s.researchField = 4;
    s.researchTurns = 11;
    map.setPanel(35001, s);
    Canvas research(640, 480, 99);
    map.draw(research, palette);
    int above = 0;
    for (int y = 391; y < 401; ++y) {
        for (int x = 560; x < 615; ++x) {
            above += research.at(x, y) == 23 && after.at(x, y) != 23 ? 1 : 0;
        }
    }
    HGM_CHECK(above > 40);
}

HGM_TEST("Galaxy map: the Enter Home Star Name box") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(game);
    NameBox box = homeStarNameBox(data, "Altair");
    HGM_CHECK(box.name() == "Altair");
    const Rect accept = box.acceptRect();
    HGM_CHECK(accept.x == 227 && accept.y == 246 && accept.w == 98 && accept.h == 28);
    HGM_CHECK(box.type('s'));
    HGM_CHECK(box.name() == "Altairs");
    HGM_CHECK(!box.click(10, 10));
    HGM_CHECK(box.click(accept.centerX(), accept.centerY()));
}

HGM_TEST("Galaxy map: fleets leaving, on their way, and the stars explored") {
    const fs::path game = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(game);
    const Galaxy g = medium();
    GalaxyMap map(data, g, {0});
    // A fleet at the home star sent to Orion: left of the star (its picture
    // is 25 wide there, the marker's corner 4 further left and 2 higher than
    // the picture's).
    GalaxyMap::Marker leaving{1, 0, 0, 0, 2};
    const Rect r = map.markerRect(leaving);
    const Rect orbiting = map.markerRect(GalaxyMap::Marker{1});
    HGM_CHECK(r.y == orbiting.y - 1 && r.x < orbiting.x - 20);
    // On its way: centred on its place, "eta 3" under it, the course in
    // greens to Orion.
    GalaxyMap::Marker flying{-1, 200, 150, 0, 3};
    const Rect f = map.markerRect(flying);
    HGM_CHECK(f.x == map.screenX(200) - 6 && f.y == map.screenY(150) - 5 && f.w == 12 && f.h == 10);
    map.setFleets({flying});
    Canvas canvas(640, 480, 99);
    Palette palette;
    map.draw(canvas, palette);
    int red = 0;
    for (int y = f.y + f.h; y < f.y + f.h + 8; ++y) {
        for (int x = f.x - 7; x < f.x + 15; ++x) {
            red += canvas.at(x, y) == 79 ? 1 : 0;
        }
    }
    HGM_CHECK(red > 15);
    int greens = 0;
    for (int x = f.x + 20; x < map.screenX(365) - 10; ++x) {
        for (int y = 100; y < 160; ++y) {
            const auto c = canvas.at(x, y);
            greens += c == 110 || c == 111 || c == 114 || c == 116 ? 1 : 0;
        }
    }
    HGM_CHECK(greens > 70);
    // Orion explored: its name in grey under it.
    map.setFleets({});
    std::vector<bool> explored(g.stars.size(), false);
    explored[0] = true;
    map.setExplored(explored);
    Canvas named(640, 480, 99);
    map.draw(named, palette);
    int grey = 0;
    for (int y = 131; y < 160; ++y) {
        for (int x = 240; x < 290; ++x) {
            grey += named.at(x, y) == 12 ? 1 : 0;
        }
    }
    HGM_CHECK(grey > 20);
}
