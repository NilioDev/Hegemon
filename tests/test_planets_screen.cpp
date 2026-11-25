// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/planets_screen.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The measured Altair: a medium radiated poor planet, the Alkari's home and
// a large toxic abundant planet; Vega, explored, far away.
Game altair(GameData& data) {
    const Races races(data);
    Galaxy g;
    g.width = 759;
    g.height = 600;
    Star a;
    a.name = "Altair";
    a.x = 669;
    a.y = 516;
    a.orbits = {0, kNoPlanet, kNoPlanet, 1, 2};
    g.stars.push_back(a);
    Star v;
    v.name = "Vega";
    v.x = 36;
    v.y = 485;
    v.orbits = {3, kNoPlanet, kNoPlanet, kNoPlanet, kNoPlanet};
    g.stars.push_back(v);
    const auto planet = [&](int star, int orbit, Climate climate, PlanetSize size, Minerals minerals) {
        Planet p;
        p.star = star;
        p.orbit = orbit;
        p.climate = climate;
        p.size = size;
        p.minerals = minerals;
        g.planets.push_back(p);
    };
    planet(0, 0, Climate::Radiated, PlanetSize::Medium, Minerals::Poor);
    planet(0, 3, Climate::Terran, PlanetSize::Medium, Minerals::Abundant);
    planet(0, 4, Climate::Toxic, PlanetSize::Large, Minerals::Abundant);
    planet(1, 0, Climate::Gaia, PlanetSize::Small, Minerals::Rich);
    g.homes.push_back({0, 0, 1});
    Game game = startGame(data.parameters(), g, {races.stock(0)}, {0});
    game.players[0].explored.assign(g.stars.size(), true);
    Ship ship;
    ship.kind = ShipKind::ColonyShip;
    ship.star = 0;
    game.ships.push_back(ship);
    return game;
}

} // namespace

HGM_TEST("Planets screen: the measured rows, sorting and filters") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = altair(data);
    PlanetsScreen screen(data, game, Economy(data.parameters()), 0);
    // Vega is out of range; radiated sorts before toxic.
    const auto& rows = screen.rows();
    HGM_CHECK(rows.size() == 2);
    HGM_CHECK(rows[0].planet == 0 && rows[1].planet == 2);
    HGM_CHECK(rows[0].climate == "Radiated" && rows[0].food == "0 Food" && rows[0].gravity == "Normal G");
    HGM_CHECK(rows[0].minerals == "Poor" && rows[0].production == "2 prod/worker");
    HGM_CHECK(rows[0].size == "Medium" && rows[0].population == "4 max pop");
    HGM_CHECK(rows[1].climate == "Toxic" && rows[1].minerals == "Abundant" && rows[1].production == "3 prod/worker");
    HGM_CHECK(rows[1].size == "Large" && rows[1].population == "5 max pop");
    HGM_CHECK(screen.mode() == PlanetsScreen::Mode::Colony);

    // Size puts the large planet first.
    const Rect size = PlanetsScreen::sortRect(PlanetsScreen::Sort::Size);
    screen.press(size.centerX(), size.centerY());
    screen.release(size.centerX(), size.centerY());
    HGM_CHECK(screen.sort() == PlanetsScreen::Sort::Size && screen.rows().front().planet == 2);
    // Without the range, Vega's gaia planet comes too; non-hostile hides
    // the others.
    const Rect range = PlanetsScreen::filterRect(PlanetsScreen::Filter::InRange);
    screen.press(range.centerX(), range.centerY());
    screen.release(range.centerX(), range.centerY());
    HGM_CHECK(screen.rows().size() == 3);
    screen.setFilter(PlanetsScreen::Filter::NonHostile, true);
    HGM_CHECK(screen.rows().size() == 1 && screen.rows().front().planet == 3);
}

HGM_TEST("Planets screen: a click on a planet sends the colony ship") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Game game = altair(data);
    PlanetsScreen screen(data, game, Economy(data.parameters()), 0);
    const Rect row = PlanetsScreen::rowRect(0);
    screen.press(row.centerX(), row.centerY());
    const auto a = screen.release(row.centerX(), row.centerY());
    HGM_CHECK(a.kind == PlanetsScreen::Action::Kind::Send && a.planet == 0 && !a.reroute &&
              game.ships.at(a.ship).kind == ShipKind::ColonyShip);
    // Measured: "0 turns to Altair", and a red square around the star.
    HGM_CHECK(screen.turnsText() == "0 turns to Altair");
    HGM_CHECK(screen.chosen() == std::optional<int>(0));
    screen.shipSent(0);
    HGM_CHECK(screen.rows().front().shipSent);
    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    HGM_CHECK(canvas.at(597, 112) == 212 && canvas.at(605, 120) == 212 && canvas.at(601, 112) == 212);
    // The colony ship button lit; a click puts it out, and a planet then
    // sends nothing.
    screen.press(PlanetsScreen::kColonyButton.centerX(), PlanetsScreen::kColonyButton.centerY());
    screen.release(PlanetsScreen::kColonyButton.centerX(), PlanetsScreen::kColonyButton.centerY());
    HGM_CHECK(screen.mode() == PlanetsScreen::Mode::None);
    screen.press(row.centerX(), row.centerY());
    HGM_CHECK(screen.release(row.centerX(), row.centerY()).kind == PlanetsScreen::Action::Kind::None);
    screen.press(PlanetsScreen::kReturnButton.centerX(), PlanetsScreen::kReturnButton.centerY());
    HGM_CHECK(screen.release(PlanetsScreen::kReturnButton.centerX(), PlanetsScreen::kReturnButton.centerY()).kind ==
              PlanetsScreen::Action::Kind::Return);
}
