// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/rules/ship_design.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/fleets_screen.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The Alkari's home at Altair with the ships every empire starts with: two
// scouts and a colony ship.
Game altair(GameData& data) {
    const Races races(data);
    Galaxy g;
    g.width = 759;
    g.height = 600;
    Star a;
    a.name = "Altair";
    a.x = 669;
    a.y = 516;
    a.orbits = {kNoPlanet, kNoPlanet, kNoPlanet, 0, kNoPlanet};
    g.stars.push_back(a);
    Planet home;
    home.star = 0;
    home.orbit = 3;
    home.climate = Climate::Terran;
    home.size = PlanetSize::Medium;
    g.planets.push_back(home);
    g.homes.push_back({0, 0, 0});
    Game game = startGame(data.parameters(), g, {races.stock(0)}, {0});
    game.players[0].race = 0;
    for (Ship& s : game.ships) {
        s.experience = 1; // as after the first turn
    }
    return game;
}

std::vector<std::string> texts(const FleetsScreen& screen) {
    std::vector<std::string> out;
    for (const auto& l : screen.panel()) {
        out.push_back(l.text);
    }
    return out;
}

bool has(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

} // namespace

HGM_TEST("Fleets screen: the measured Alkari fleet, a scout and the colony ship") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const TechTree tree(data.parameters());
    const Game game = altair(data);
    FleetsScreen screen(data, game, tree, 0);
    HGM_CHECK(screen.fleets().size() == 1);
    HGM_CHECK(screen.title() == "ALKARI FLEET: 1 Colony Ship, 2 Frigates");
    HGM_CHECK(screen.shown().size() == 3);
    // Nothing chosen at first, and Scrap is grey.
    HGM_CHECK(!screen.chosen() && screen.panel().empty() && screen.selected().empty());

    // The first ship is a scout: measured "+25" and "+110" for the Alkari.
    const auto scout = std::find_if(screen.shown().begin(), screen.shown().end(), [&](std::size_t i) {
        return game.ships[i].kind == ShipKind::Scout;
    });
    HGM_CHECK(scout != screen.shown().end());
    const Rect cell = FleetsScreen::cellRect(static_cast<std::size_t>(scout - screen.shown().begin()));
    screen.press(cell.centerX(), cell.centerY());
    screen.release(cell.centerX(), cell.centerY());
    HGM_CHECK(screen.chosen() == std::optional<std::size_t>(*scout) && screen.selected().size() == 1);
    const auto lines = texts(screen);
    HGM_CHECK(has(lines, "Scout") && has(lines, "Green Crew (1 EP)") && has(lines, "No Shield"));
    HGM_CHECK(has(lines, "+25") && has(lines, "+110"));
    HGM_CHECK(has(lines, "1 Laser Cannon (F)") && has(lines, "Extended Fuel Tanks"));
    const auto& name = screen.panel().front();
    HGM_CHECK(name.x == 18 && name.y == 287 && name.font == 3);

    // The colony ship: its help text in four lines from y 344.
    const auto colony = std::find_if(screen.shown().begin(), screen.shown().end(), [&](std::size_t i) {
        return game.ships[i].kind == ShipKind::ColonyShip;
    });
    HGM_CHECK(colony != screen.shown().end());
    const Rect c2 = FleetsScreen::cellRect(static_cast<std::size_t>(colony - screen.shown().begin()));
    screen.press(c2.centerX(), c2.centerY());
    screen.release(c2.centerX(), c2.centerY());
    HGM_CHECK(screen.selected().size() == 2 && screen.panel().size() == 4);
    HGM_CHECK(screen.panel().front().text == "Capable of creating a colony in a distant" &&
              screen.panel().front().y == 344);
    // A second click puts it out but keeps its text.
    screen.press(c2.centerX(), c2.centerY());
    screen.release(c2.centerX(), c2.centerY());
    HGM_CHECK(screen.selected().size() == 1 && screen.panel().size() == 4);
    // All lights them all.
    screen.press(FleetsScreen::kAllButton.centerX(), FleetsScreen::kAllButton.centerY());
    screen.release(FleetsScreen::kAllButton.centerX(), FleetsScreen::kAllButton.centerY());
    HGM_CHECK(screen.selected().size() == 3);

    // Support off hides the colony ship; Combat off then brings Support back.
    screen.setSupport(false);
    HGM_CHECK(screen.shown().size() == 2 && screen.selected().size() == 2);
    screen.setCombat(false);
    HGM_CHECK(screen.support() && !screen.combat() && screen.shown().size() == 1);

    Canvas canvas(640, 480);
    Palette palette;
    screen.draw(canvas, palette);
    // The scroll bar lit for a fleet that fits.
    HGM_CHECK(canvas.at(607, 200) == 165 && canvas.at(616, 200) == 161);
    screen.press(FleetsScreen::kReturnButton.centerX(), FleetsScreen::kReturnButton.centerY());
    HGM_CHECK(screen.release(FleetsScreen::kReturnButton.centerX(), FleetsScreen::kReturnButton.centerY()) ==
              FleetsScreen::Action::Return);
}

HGM_TEST("Ships: crew levels, beam attack and defense") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    HGM_CHECK(crewLevel(p, 0) == 0 && crewLevel(p, 49) == 0 && crewLevel(p, 50) == 1 && crewLevel(p, 1000) == 4);
    const Races races(data);
    const RacePicks alkari = races.stock(0);
    ShipDesign scout;
    scout.computer = 1;
    scout.combatSpeed = 12;
    // Measured with edited ships: an optronic computer +50, combat speed 20
    // +150.
    HGM_CHECK(beamAttack(p, alkari, scout, 0) == 25 && beamDefense(p, alkari, scout, 0) == 110);
    scout.computer = 2;
    scout.combatSpeed = 20;
    HGM_CHECK(beamAttack(p, alkari, scout, 0) == 50 && beamDefense(p, alkari, scout, 0) == 150);
}
