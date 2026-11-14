// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/rules/fleet.hpp"

#include <string>
#include <vector>

using namespace hegemon;

namespace {

const std::string kList = R"({"patch_version": "test", "parameters": [
 {"name": "drive", "type": "table", "rows": [["nuclear_drive", "fusion_drive"]],
  "default": [[2], [3]], "columns": [{"name": "warp_speed"}]}
]})";

Game twoStars() {
    Game game;
    Star altair;
    altair.x = 669;
    altair.y = 516;
    Star sal;
    sal.x = 607;
    sal.y = 437;
    game.galaxy.stars = {altair, sal};
    game.players.resize(1);
    game.players[0].explored = {true, false};
    game.ships = {{0, ShipKind::Scout, 0}, {0, ShipKind::Scout, 0}, {0, ShipKind::ColonyShip, 0}};
    return game;
}

} // namespace

HGM_TEST("Fleets: a turn's way, as measured in the original") {
    struct Step {
        MapPoint from, to, after;
    };
    // Ships of every player in one game, a turn at a time at 2 parsecs.
    const std::vector<Step> steps = {
        {{146, 62}, {285, 129}, {201, 89}},  {{201, 89}, {285, 129}, {256, 115}},
        {{256, 115}, {285, 129}, {285, 129}}, {{146, 62}, {65, 45}, {86, 49}},
        {{65, 45}, {146, 62}, {125, 58}},     {{317, 402}, {269, 558}, {299, 460}},
        {{299, 460}, {269, 558}, {281, 518}}, {{317, 402}, {354, 328}, {345, 347}},
        {{669, 516}, {607, 437}, {631, 468}}, {{631, 468}, {607, 437}, {607, 437}},
        {{579, 147}, {624, 231}, {608, 201}}, {{579, 147}, {691, 61}, {627, 110}},
    };
    for (const Step& s : steps) {
        HGM_CHECK(stepTowards(s.from, s.to, 2 * kParsec) == s.after);
    }
    // The ETA: 100 units at 2 parsecs is 2 turns, 93 is 2, 32 is 1.
    HGM_CHECK(travelTurns({669, 516}, {607, 437}, 2) == 2);
    HGM_CHECK(travelTurns({201, 89}, {285, 129}, 2) == 2);
    HGM_CHECK(travelTurns({256, 115}, {285, 129}, 2) == 1);
    HGM_CHECK(travelTurns({146, 62}, {285, 129}, 2) == 3);
}

HGM_TEST("Fleets: ships sent off travel together and explore the star") {
    const Parameters p = Parameters::defaults(kList);
    HGM_CHECK(warpSpeed(p, 1) == 2 && warpSpeed(p, 2) == 3);
    Game game = twoStars();
    HGM_CHECK(fleets(game).size() == 1);
    const std::vector<std::size_t> two = {0, 2};
    orderShips(game, p, two, 1);
    auto f = fleets(game);
    HGM_CHECK(f.size() == 2 && f[0].ships == two && f[0].destination == 1 && f[0].star == 0);
    moveShips(game);
    HGM_CHECK(game.ships[0].star == -1 && (shipPlace(game, game.ships[0]) == MapPoint{631, 468}));
    HGM_CHECK(game.ships[1].star == 0);
    HGM_CHECK(!explored(game.players[0], 1));
    moveShips(game);
    HGM_CHECK(game.ships[0].star == 1 && game.ships[2].star == 1 && game.ships[0].destination == -1);
    HGM_CHECK(explored(game.players[0], 1));
    // Sent to the star it is at: no order.
    const std::vector<std::size_t> one = {1};
    orderShips(game, p, one, 0);
    HGM_CHECK(game.ships[1].destination == -1);
}
