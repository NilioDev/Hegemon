// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/build_list.hpp"
#include "hegemon/ui/colony_screen.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

using List = std::vector<std::string>;

// The Alkari's home system: their planet and a free one beside it.
Game alkariGame(GameData& data) {
    const Races races(data);
    Galaxy g;
    Star s;
    s.name = "Altair";
    s.orbits = {0, 1, kNoPlanet, kNoPlanet, kNoPlanet};
    g.stars.push_back(s);
    Planet home;
    home.climate = Climate::Terran;
    Planet other;
    other.orbit = 1;
    g.planets.push_back(home);
    g.planets.push_back(other);
    g.homes.push_back({0, 0, 0});
    return startGame(data.parameters(), g, {races.stock(0)}, {0});
}

int count(const Canvas& c, std::uint8_t colour, Rect r) {
    int n = 0;
    for (int y = r.y; y < r.y + r.h; ++y) {
        for (int x = r.x; x < r.x + r.w; ++x) {
            n += c.at(x, y) == colour ? 1 : 0;
        }
    }
    return n;
}

} // namespace

HGM_TEST("Build list: ships, spies, housing and trade goods repeat; buildings don't") {
    Colony c;
    HGM_CHECK(addToBuildList(c, kSpy) && addToBuildList(c, kSpy) && addToBuildList(c, kHousing));
    HGM_CHECK(addToBuildList(c, "automated_factory"));
    HGM_CHECK(!addToBuildList(c, "automated_factory"));
    HGM_CHECK(addToBuildList(c, kColonyShip) && addToBuildList(c, kTradeGoods) && addToBuildList(c, kFreighters));
    // Seven lines at most.
    HGM_CHECK(c.buildList.size() == Colony::kBuildListLength);
    HGM_CHECK(!addToBuildList(c, kSpy));
    HGM_CHECK(repeatable(kOutpostShip) && repeatable(kTransportShip) && !repeatable(kColonyBase));
}

HGM_TEST("Build list: what a new home colony can build, and what it costs") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = alkariGame(data);
    const Parameters& p = data.parameters();
    // Measured: at an average start every building it knows stands already.
    const Buildable b = buildable(p, game, 0);
    HGM_CHECK((b.buildings == List{"trade_goods", "housing"}));
    HGM_CHECK((b.others == List{"colony_base", "freighters", "colony_ship", "outpost_ship", "transport_ship", "spy"}));
    HGM_CHECK(productCost(p, kColonyShip) == 500 && productCost(p, kSpy) == 100 && productCost(p, kHousing) == 0);
    HGM_CHECK(productCost(p, kOutpostShip) == 100 && productCost(p, kTransportShip) == 100);
    // A system with no other planet has no room for a colony base.
    Game alone = game;
    alone.galaxy.stars[0].orbits[1] = kNoPlanet;
    HGM_CHECK(buildable(p, alone, 0).others.front() == "freighters");
}

HGM_TEST("Build list: the lists, the box at the top and the build list") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = alkariGame(data);
    const Economy economy(data.parameters());
    const ColonyScreen colony(data, game, economy, 0);
    BuildListScreen screen(data, game, economy, 0, colony.scene(), colony.palette());
    HGM_CHECK((screen.left() == List{"trade_goods", "housing"}));
    HGM_CHECK(screen.name("trade_goods") == "Trade Goods" && screen.name("housing") == "Housing");
    const auto& right = screen.right();
    HGM_CHECK(screen.name(right[0]) == "Colony Base" && screen.name(right[1]) == "Freighter Fleet");
    HGM_CHECK(screen.name(right[2]) == "Colony Ship" && screen.name(right[3]) == "Outpost Ship");
    HGM_CHECK(screen.name(right[4]) == "Transport Ship" && right[5].empty() && screen.name(right[13]) == "Spy");

    // The turns rounded up.
    const ColonyOutput out = economy.output(game, game.colonies[0]);
    const int production = out.production - out.upkeep;
    screen.hover(500, 272);
    HGM_CHECK(screen.hoverName() == "Spy" && screen.costText() == "Cost Here: 100");
    HGM_CHECK(screen.timeText() == "Build Time: " + std::to_string((100 + production - 1) / production));
    screen.hover(40, 44);
    HGM_CHECK(screen.hoverName() == "Housing" && screen.costText().empty());
    screen.hover(300, 200);
    HGM_CHECK(screen.hoverName().empty());

    // Clicks add to the end; a click on a line deletes it.
    for (const auto& [x, y] : {std::pair{500, 272}, {500, 272}, {40, 44}, {40, 26}}) {
        screen.press(x, y);
        HGM_CHECK(screen.release(x, y) == BuildListScreen::Result::None);
    }
    HGM_CHECK((screen.buildList() == List{"spy", "spy", "housing", "trade_goods"}));
    screen.hover(330, 340);
    HGM_CHECK(screen.hoverName() == "Spy");
    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    // The line the pointer is on is framed in red; the products in the
    // build list are bright, those not in it darker.
    HGM_CHECK(count(canvas, 239, {207, 332, 252, 16}) > 500);
    HGM_CHECK((palette.colors[187] == Color{231, 138, 32}) && (palette.colors[203] == Color{174, 85, 44}));
    HGM_CHECK(count(canvas, 187, {13, 20, 170, 36}) > 300 && count(canvas, 203, {13, 20, 170, 36}) == 0);
    HGM_CHECK(count(canvas, 0, {8, 100, 180, 100}) == 0);
    screen.press(330, 340);
    screen.release(330, 340);
    HGM_CHECK((screen.buildList() == List{"spy", "housing", "trade_goods"}));

    // The keyboard and gamepad move a highlight.
    screen.hover(0, 0);
    screen.moveHighlight(0, 1);
    HGM_CHECK(screen.hoverName() == "Trade Goods");
    screen.moveHighlight(0, 1);
    screen.choose();
    HGM_CHECK(screen.buildList().back() == "housing" && screen.buildList().size() == 4);
    screen.moveHighlight(1, 0);
    HGM_CHECK(screen.hovered() && screen.hovered()->column == BuildListScreen::Column::Queue);
    screen.moveHighlight(1, 0);
    HGM_CHECK(screen.hoverName() == "Freighter Fleet");

    screen.press(580, 455);
    HGM_CHECK(screen.release(580, 455) == BuildListScreen::Result::Ok);
    screen.press(510, 455);
    HGM_CHECK(screen.release(510, 455) == BuildListScreen::Result::Cancel);
    screen.press(510, 455);
    HGM_CHECK(screen.release(300, 200) == BuildListScreen::Result::None);
}
