// SPDX-License-Identifier: GPL-3.0-or-later
//
// Ships travelling between the stars: fleets, orders and how far a fleet
// goes in a turn. See docs/rules/fleets.md.
#pragma once

#include "hegemon/rules/empire.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace hegemon {

class Parameters;

// Map units in a parsec (the fan patch's warp_interdictor_range is three
// parsecs squared).
constexpr int kParsec = 30;

struct MapPoint {
    int x = 0, y = 0;
    bool operator==(const MapPoint&) const = default;
};

// A player's ships at one place with one order: those at a star with no
// order, those at a star sent to the same star, or those travelling
// together.
struct Fleet {
    int player = 0;
    int star = -1;        // where it is, -1 while it travels
    int destination = -1; // where it goes, -1 for none
    MapPoint place;       // on the map
    std::vector<std::size_t> ships; // in the game's list
};

// The parsecs a turn of a drive (the drive table's row + 1).
int warpSpeed(const Parameters& parameters, int drive);
// The parsecs a turn a ship can go: its drive's, for a ship of a design
// the design's; the starting ships and the landing ships have the drive of
// the player's first design (assumed: the best drive the player had at the
// start).
int shipSpeed(const Parameters& parameters, const Game& game, const Ship& ship);

// Where a ship is on the map.
MapPoint shipPlace(const Game& game, const Ship& ship);

// The distance the original goes by: the whole part of the straight line.
int mapDistance(MapPoint from, MapPoint to);
// The turns it takes to go from one place to another at `speed` parsecs a
// turn (the fleet window's ETA): at least one.
int travelTurns(MapPoint from, MapPoint to, int speed);
// One turn's way towards `to`: there if it is no further than `step` map
// units, otherwise each coordinate goes its share of the step, rounded up.
MapPoint stepTowards(MapPoint from, MapPoint to, int step);

// The fleets of every player, in the order of their first ship.
std::vector<Fleet> fleets(const Game& game);

// Sends ships to a star: they leave at the end of the turn, together at
// the slowest one's speed. Sending them to the star they are at takes the
// order back.
void orderShips(Game& game, const Parameters& parameters, std::span<const std::size_t> ships, int star);

// The ships' journeys at the end of the turn: each travelling one goes a
// turn's way, and one that arrives is at its star and the player has
// explored it.
void moveShips(Game& game);

// Whether a player has explored a star.
bool explored(const Player& player, int star);

} // namespace hegemon
