// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/colonies_screen.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The Alkari's home at Altair, as in the measured game, and a second
// colony at Vega.
Game twoColonies(GameData& data) {
    const Races races(data);
    Galaxy g;
    g.width = 759;
    g.height = 600;
    Star altair;
    altair.name = "Altair";
    altair.x = 669;
    altair.y = 516;
    altair.orbits[0] = 0;
    g.stars.push_back(altair);
    Star vega;
    vega.name = "Vega";
    vega.x = 595;
    vega.y = 560;
    vega.orbits[0] = 1;
    g.stars.push_back(vega);
    Star hole;
    hole.colour = StarColour::BlackHole;
    hole.x = 527;
    hole.y = 35;
    g.stars.push_back(hole);
    Planet home;
    home.climate = Climate::Terran;
    home.size = PlanetSize::Medium;
    home.minerals = Minerals::Abundant;
    g.planets.push_back(home);
    Planet other = home;
    other.star = 1;
    other.climate = Climate::Desert;
    other.size = PlanetSize::Small;
    g.planets.push_back(other);
    g.homes.push_back({0, 0, 0});
    Game game = startGame(data.parameters(), g, {races.stock(0)}, {0});
    game.colonies[0].buildList = {std::string(kColonyBase)};
    Colony second = game.colonies[0];
    second.planet = 1;
    second.jobs = {1, 0, 1};
    second.buildList.clear();
    game.colonies.push_back(second);
    return game;
}

} // namespace

HGM_TEST("Colonies screen: a row for each colony, as measured") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = twoColonies(data);
    const Economy economy(data.parameters());
    ColoniesScreen screen(data, game, economy, 0);
    HGM_CHECK((screen.order() == std::vector<std::size_t>{0, 1}));
    HGM_CHECK(screen.name(0) == "Altair Prime");
    // As the colony screen says it.
    const ColonyScreen colony(data, game, economy, 0);
    HGM_CHECK(screen.building(0) == "Colony Base - " + colony.turnsText().substr(0, colony.turnsText().find(' ')) + "t");
    HGM_CHECK(screen.building(1).empty());
    HGM_CHECK(screen.shown() == std::optional<std::size_t>(0));
    const auto& d = screen.details();
    HGM_CHECK(d.size() == 7);
    // Measured on the Alkari's home, but for what is made.
    HGM_CHECK(d[0] == "Medium Terran" && d[1] == "Normal Gravity" && d[2] == "Mineral Abundant");
    HGM_CHECK(d[3] == "Population (8/12)" && d[4] == "Enroute (0)" && d[5] == "0k +73k (0)");
    HGM_CHECK(d[6].rfind("0/200 +", 0) == 0);
    HGM_CHECK(screen.totals().size() == 6);
    HGM_CHECK(screen.totals()[0].first == "Reserve:");
    HGM_CHECK(screen.totals()[2].second == "10");

    // The details follow the pointer over the rows.
    screen.pointerMoved(40, 35 + 31 + 10);
    HGM_CHECK(screen.shown() == std::optional<std::size_t>(1));

    // Sorting by population puts the larger colony first; by name, Altair.
    screen.press(150, 455);
    HGM_CHECK(screen.release(150, 455).kind == ColoniesScreen::Action::Kind::None);
    HGM_CHECK(screen.sort() == ColoniesScreen::Sort::Population);
    HGM_CHECK((screen.order() == std::vector<std::size_t>{0, 1}));
    screen.setSort(ColoniesScreen::Sort::Science);
    HGM_CHECK(screen.order().front() == 0);

    // The name opens the colony, what is built its build list, the round
    // button buys it; Return goes back.
    using Kind = ColoniesScreen::Action::Kind;
    screen.press(40, 50);
    auto a = screen.release(40, 50);
    HGM_CHECK(a.kind == Kind::Open && a.colony == 0);
    screen.press(540, 82);
    a = screen.release(540, 82);
    HGM_CHECK(a.kind == Kind::Build && a.colony == 1);
    screen.press(608, 50);
    a = screen.release(608, 50);
    HGM_CHECK(a.kind == Kind::Buy && a.colony == 0);
    screen.press(560, 455);
    HGM_CHECK(screen.release(560, 455).kind == Kind::Return);
    // Rows without a colony do nothing.
    screen.press(40, 120);
    HGM_CHECK(screen.release(40, 120).kind == Kind::None);

    // A worker taken to the farmers.
    const auto before = screen.jobs(0);
    screen.press(240, 45);
    HGM_CHECK(screen.heldCount() == before[1]);
    a = screen.release(120, 45);
    HGM_CHECK(a.kind == Kind::Jobs && a.colony == 0);
    HGM_CHECK(screen.jobs(0)[0] == before[0] + before[1] && screen.jobs(0)[1] == 0);

    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    // The small map: Altair's star in red, the black hole a ring.
    HGM_CHECK(canvas.at(492, 427) == 239 && canvas.at(491, 427) == 238);
    HGM_CHECK(canvas.at(468, 354) == 120 && canvas.at(467, 354) == 124);
    HGM_CHECK(canvas.at(480, 433) == 135);
}

HGM_TEST("Colonies screen: the pointer steps over rows and buttons") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = twoColonies(data);
    ColoniesScreen screen(data, game, Economy(data.parameters()), 0);
    const Rect name0 = ColoniesScreen::cellRect(0, ColoniesScreen::Column::Name);
    auto [x, y] = screen.step(name0.centerX(), name0.centerY(), 0, 1);
    HGM_CHECK(ColoniesScreen::cellRect(1, ColoniesScreen::Column::Name).contains(x, y));
    std::tie(x, y) = screen.step(x, y, 0, 1);
    HGM_CHECK(ColoniesScreen::sortRect(ColoniesScreen::Sort::Name).contains(x, y));
    std::tie(x, y) = screen.step(x, y, 0, 1);
    HGM_CHECK(ColoniesScreen::sortRect(ColoniesScreen::Sort::Name).contains(x, y));
    std::tie(x, y) = screen.step(name0.centerX(), name0.centerY(), 5, 0);
    HGM_CHECK(ColoniesScreen::cellRect(0, ColoniesScreen::Column::Buy).contains(x, y));
}
