// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/game_data.hpp"
#include "hegemon/game_files.hpp"
#include "hegemon/rules/computer_players.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/rules/random.hpp"
#include "hegemon/rules/tech.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace hegemon;

namespace {

// The human at Sol, a computer player at Altair; near Altair, Vega with a
// barren planet that holds more people and a tundra one; further off Deneb
// with a terran planet.
Game galaxy(GameData& data) {
    const Races races(data);
    Galaxy g;
    g.width = 759;
    g.height = 600;
    const auto addStar = [&](const char* name, int x, int y) {
        Star s;
        s.name = name;
        s.x = x;
        s.y = y;
        s.orbits = {kNoPlanet, kNoPlanet, kNoPlanet, kNoPlanet, kNoPlanet};
        g.stars.push_back(s);
    };
    const auto planet = [&](int at, int orbit, Climate climate, PlanetSize size) {
        Planet p;
        p.star = at;
        p.orbit = orbit;
        p.climate = climate;
        p.size = size;
        g.stars[static_cast<std::size_t>(at)].orbits[static_cast<std::size_t>(orbit)] =
            static_cast<int>(g.planets.size());
        g.planets.push_back(p);
    };
    addStar("Sol", 60, 60);
    addStar("Altair", 600, 500);
    addStar("Vega", 650, 430);
    addStar("Deneb", 400, 300);
    planet(0, 2, Climate::Terran, PlanetSize::Medium);
    planet(1, 2, Climate::Terran, PlanetSize::Medium);
    planet(2, 0, Climate::Barren, PlanetSize::Large);
    planet(2, 1, Climate::Tundra, PlanetSize::Small);
    planet(3, 1, Climate::Terran, PlanetSize::Large);
    g.homes.push_back({0, 0, 0});
    g.homes.push_back({1, 1, 1});
    return startGame(data.parameters(), g, {races.stock(0), races.stock(1)}, {0, 1});
}

} // namespace

HGM_TEST("Computer players: research, building, scouts and the colony ship") {
    const fs::path folder = HEGEMON_GAMEDATA_DIR;
    if (!isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    GameData data(folder);
    const Parameters& p = data.parameters();
    const TechTree tree(p);
    Game game = galaxy(data);
    HGM_CHECK(game.players[0].human && !game.players[1].human);
    game.players[1].research = 0;
    for (Colony& c : game.colonies) {
        c.buildList.clear();
    }
    Random random(1);
    orderComputerPlayers(game, p, tree, random);
    // The cheapest field, trade goods at home; the human's colony untouched.
    Player& ai = game.players[1];
    HGM_CHECK(ai.research > 0 && tree.field(ai.research).cost == 80);
    for (const Colony& c : game.colonies) {
        HGM_CHECK(c.player == 0 ? c.buildList.empty() : c.buildList == std::vector<std::string>{"trade_goods"});
    }
    // The colony ship goes to the nearest other star, to the planet one can
    // farm; the scouts to the two other stars.
    const auto colonyShip = std::find_if(game.ships.begin(), game.ships.end(), [](const Ship& s) {
        return s.player == 1 && s.kind == ShipKind::ColonyShip;
    });
    HGM_CHECK(colonyShip != game.ships.end() && colonyShip->destination == 2 && colonyShip->landOn == 3);
    std::vector<int> scouted;
    for (const Ship& s : game.ships) {
        if (s.player == 1 && s.kind == ShipKind::Scout) {
            scouted.push_back(s.destination);
        }
        if (s.player == 0) {
            HGM_CHECK(s.destination < 0);
        }
    }
    std::sort(scouted.begin(), scouted.end());
    HGM_CHECK(scouted == (std::vector<int>{0, 3}));
    // It lands in the turn it arrives.
    for (int t = 0; t < 10 && std::none_of(game.colonies.begin(), game.colonies.end(),
                                           [](const Colony& c) { return c.planet == 3; });
         ++t) {
        endTurn(game, p, tree, random);
    }
    const auto settled = std::find_if(game.colonies.begin(), game.colonies.end(),
                                      [](const Colony& c) { return c.planet == 3; });
    HGM_CHECK(settled != game.colonies.end() && settled->player == 1);
    HGM_CHECK(std::none_of(game.ships.begin(), game.ships.end(), [](const Ship& s) {
        return s.player == 1 && s.kind == ShipKind::ColonyShip;
    }));
}
