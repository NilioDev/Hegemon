// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/fleet.hpp"

#include "hegemon/config/parameters.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace hegemon {

namespace {

// The whole part of a square root.
std::int64_t wholeRoot(std::int64_t n) {
    if (n <= 0) {
        return 0;
    }
    auto r = static_cast<std::int64_t>(std::sqrt(static_cast<double>(n)));
    while (r * r > n) {
        --r;
    }
    while ((r + 1) * (r + 1) <= n) {
        ++r;
    }
    return r;
}

// Rounded up, for numbers not below 0.
std::int64_t dividedUp(std::int64_t a, std::int64_t b) { return (a + b - 1) / b; }

bool sameOrder(const Ship& a, const Ship& b) {
    return a.player == b.player && a.star == b.star && a.destination == b.destination &&
           (a.star >= 0 || (a.x == b.x && a.y == b.y));
}

} // namespace

int warpSpeed(const Parameters& parameters, int drive) {
    const ParameterDefinition* table = parameters.definition("drive");
    if (table == nullptr || drive < 1 || static_cast<std::size_t>(drive) > table->rowCount()) {
        return 1;
    }
    const auto& columns = table->columns;
    const auto column = std::find(columns.begin(), columns.end(), "warp_speed");
    if (column == columns.end()) {
        return 1;
    }
    return std::max(1, static_cast<int>(parameters.integer("drive", static_cast<std::size_t>(drive - 1),
                                                           static_cast<std::size_t>(column - columns.begin()))));
}

int shipSpeed(const Parameters& parameters, const Game& game, const Ship& ship) {
    if (ship.player < 0 || static_cast<std::size_t>(ship.player) >= game.players.size()) {
        return warpSpeed(parameters, 1);
    }
    const auto& designs = game.players[static_cast<std::size_t>(ship.player)].designs;
    if (ship.kind == ShipKind::Combat && ship.design >= 0 && static_cast<std::size_t>(ship.design) < designs.size()) {
        return warpSpeed(parameters, designs[static_cast<std::size_t>(ship.design)].drive);
    }
    return warpSpeed(parameters, designs.empty() ? 1 : designs.front().drive);
}

MapPoint shipPlace(const Game& game, const Ship& ship) {
    if (ship.star >= 0 && static_cast<std::size_t>(ship.star) < game.galaxy.stars.size()) {
        const Star& s = game.galaxy.stars[static_cast<std::size_t>(ship.star)];
        return {s.x, s.y};
    }
    return {ship.x, ship.y};
}

int mapDistance(MapPoint from, MapPoint to) {
    const std::int64_t dx = to.x - from.x, dy = to.y - from.y;
    return static_cast<int>(wholeRoot(dx * dx + dy * dy));
}

int travelTurns(MapPoint from, MapPoint to, int speed) {
    const int step = std::max(1, speed) * kParsec;
    return std::max(1, static_cast<int>(dividedUp(mapDistance(from, to), step)));
}

MapPoint stepTowards(MapPoint from, MapPoint to, int step) {
    const int d = mapDistance(from, to);
    if (d <= step) {
        return to;
    }
    const auto share = [&](int a, int b) {
        const std::int64_t delta = b - a;
        const std::int64_t moved = dividedUp(std::llabs(delta) * step, d);
        return a + static_cast<int>(delta < 0 ? -moved : moved);
    };
    return {share(from.x, to.x), share(from.y, to.y)};
}

std::vector<Fleet> fleets(const Game& game) {
    std::vector<Fleet> result;
    for (std::size_t i = 0; i < game.ships.size(); ++i) {
        const Ship& ship = game.ships[i];
        const auto same = std::find_if(result.begin(), result.end(), [&](const Fleet& f) {
            return sameOrder(game.ships[f.ships.front()], ship);
        });
        if (same != result.end()) {
            same->ships.push_back(i);
            continue;
        }
        Fleet f;
        f.player = ship.player;
        f.star = ship.star;
        f.destination = ship.destination;
        f.place = shipPlace(game, ship);
        f.ships.push_back(i);
        result.push_back(std::move(f));
    }
    return result;
}

void orderShips(Game& game, const Parameters& parameters, std::span<const std::size_t> ships, int star) {
    if (star < 0 || static_cast<std::size_t>(star) >= game.galaxy.stars.size()) {
        return;
    }
    int speed = 0;
    for (const std::size_t i : ships) {
        if (i < game.ships.size()) {
            const int s = shipSpeed(parameters, game, game.ships[i]);
            speed = speed == 0 ? s : std::min(speed, s);
        }
    }
    for (const std::size_t i : ships) {
        if (i >= game.ships.size()) {
            continue;
        }
        Ship& ship = game.ships[i];
        ship.speed = speed;
        ship.destination = star == ship.star ? -1 : star;
    }
}

void moveShips(Game& game) {
    for (Ship& ship : game.ships) {
        if (ship.destination < 0 || static_cast<std::size_t>(ship.destination) >= game.galaxy.stars.size()) {
            continue;
        }
        const Star& target = game.galaxy.stars[static_cast<std::size_t>(ship.destination)];
        const MapPoint at = stepTowards(shipPlace(game, ship), {target.x, target.y}, std::max(1, ship.speed) * kParsec);
        if (at == MapPoint{target.x, target.y}) {
            ship.star = ship.destination;
            ship.destination = -1;
            if (ship.player >= 0 && static_cast<std::size_t>(ship.player) < game.players.size()) {
                auto& seen = game.players[static_cast<std::size_t>(ship.player)].explored;
                if (seen.size() < game.galaxy.stars.size()) {
                    seen.resize(game.galaxy.stars.size(), false);
                }
                seen[static_cast<std::size_t>(ship.star)] = true;
            }
        } else {
            ship.star = -1;
        }
        ship.x = at.x;
        ship.y = at.y;
    }
}

bool explored(const Player& player, int star) {
    return star >= 0 && static_cast<std::size_t>(star) < player.explored.size() &&
           player.explored[static_cast<std::size_t>(star)];
}

} // namespace hegemon
