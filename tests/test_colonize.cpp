// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/empire.hpp"
#include "hegemon/rules/fleet.hpp"
#include "hegemon/ui/colonize_box.hpp"
#include "hegemon/ui/fleet_window.hpp"
#include "hegemon/ui/landing_screen.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The measured landing: Alkari with a colony ship at Sal, a large poor
// ocean planet in the second orbit, a gas giant in the third and a huge
// barren planet in the fifth.
Game salGame() {
    Game game;
    Star sal;
    sal.name = "Sal";
    sal.orbits = {kNoPlanet, 0, 1, kNoPlanet, 2};
    game.galaxy.stars = {sal};
    Planet ocean;
    ocean.orbit = 1;
    ocean.size = PlanetSize::Large;
    ocean.climate = Climate::Ocean;
    ocean.minerals = Minerals::Poor;
    Planet giant;
    giant.orbit = 2;
    giant.type = SatelliteType::GasGiant;
    Planet barren;
    barren.orbit = 4;
    barren.size = PlanetSize::Huge;
    barren.climate = Climate::Barren;
    game.galaxy.planets = {ocean, giant, barren};
    Player alkari;
    alkari.race = 0;
    alkari.picks.values[RacePicks::Government] = 2; // dictatorship
    alkari.explored = {true};
    ShipDesign scout;
    scout.image = 5;
    alkari.designs = {scout};
    game.players = {alkari};
    game.ships = {{0, ShipKind::Scout, 0}, {0, ShipKind::ColonyShip, 0}};
    return game;
}

} // namespace

HGM_TEST("Colonizing: the planets a colony ship can settle, and the new colony") {
    Game game = salGame();
    HGM_CHECK((colonySites(game, 0) == std::vector<int>{0, 2}));
    const std::size_t colony = foundColony(game, 1, 0);
    HGM_CHECK(colony == 0 && game.colonies.size() == 1);
    // Measured: one worker on Sal Prime; the colony ship is gone.
    HGM_CHECK(game.colonies[0].planet == 0 && game.colonies[0].population() == 1);
    HGM_CHECK(game.colonies[0].jobs[static_cast<std::size_t>(Job::Worker)] == 1);
    HGM_CHECK(game.ships.size() == 1 && game.ships[0].kind == ShipKind::Scout);
    HGM_CHECK((colonySites(game, 0) == std::vector<int>{2}));
}

HGM_TEST("Colonizing: the measured question and landing on Sal Prime") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    Game game = salGame();

    // The fleet window offers Colonize Planet under Close.
    FleetWindow window(data, game, fleets(game).at(0), 0);
    HGM_CHECK(window.colonizeButton().has_value());
    const Rect colonize = *window.colonizeButton();
    window.press(colonize.centerX(), colonize.centerY());
    HGM_CHECK(window.release(colonize.centerX(), colonize.centerY()) == FleetWindow::Action::Colonize);

    ColonizeBox box(data, game, 0, 0);
    HGM_CHECK(box.title() == "Build colony on Sal Prime with");
    const auto& rows = box.rows();
    HGM_CHECK(rows.size() == 6);
    if (rows.size() == 6) {
        // Measured values for the Alkari.
        HGM_CHECK(rows[0].second == "2" && rows[1].second == "2" && rows[2].second == "3");
        HGM_CHECK(rows[3].second == "+0%" && rows[4].second == "+0%" && rows[5].second == "5");
    }
    box.press(ColonizeBox::kNo.centerX(), ColonizeBox::kNo.centerY());
    HGM_CHECK(box.release(ColonizeBox::kNo.centerX(), ColonizeBox::kNo.centerY()) == ColonizeBox::Answer::No);
    box.press(ColonizeBox::kYes.centerX(), ColonizeBox::kYes.centerY());
    HGM_CHECK(box.release(ColonizeBox::kYes.centerX(), ColonizeBox::kYes.centerY()) == ColonizeBox::Answer::Yes);

    const std::size_t colony = foundColony(game, 1, 0);
    LandingScreen landing(data, game, colony);
    HGM_CHECK(landing.title() == "Colony built on Sal Prime");
    HGM_CHECK(!landing.landed());
    landing.skip();
    HGM_CHECK(landing.landed());
    const std::vector<std::string> lines = {"Sal Prime - Alkari colony", "Large Ocean", "5 Total Population",
                                            "Mineral Poor", "Normal G"};
    HGM_CHECK(landing.lines() == lines);
    Canvas canvas(640, 480, 0);
    Palette palette;
    landing.draw(canvas, palette);
}
