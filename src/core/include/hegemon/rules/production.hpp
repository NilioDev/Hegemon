// SPDX-License-Identifier: GPL-3.0-or-later
//
// What a colony can build: the products of its build list and what they
// cost, read from the fan patch's rule settings. See
// docs/rules/colony-screen.md.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

class Parameters;
struct Game;
struct Colony;

// Products that are no building, by the names the fan patch's settings
// give them where they have one.
inline constexpr std::string_view kTradeGoods = "trade_goods";
inline constexpr std::string_view kHousing = "housing";
inline constexpr std::string_view kColonyBase = "colony_base";
inline constexpr std::string_view kFreighters = "freighters";
inline constexpr std::string_view kColonyShip = "colony_ship";
inline constexpr std::string_view kOutpostShip = "outpost_ship";
inline constexpr std::string_view kTransportShip = "transport_ship";
inline constexpr std::string_view kSpy = "spy";

// What a colony can add to its build list now, as the build list shows it.
struct Buildable {
    // Trade goods, housing and the buildings, in the order of the fan
    // patch's prod_format (products of the same order still need sorting
    // by their names).
    std::vector<std::string> buildings;
    // Colony bases, freighters, the landing ships and spies.
    std::vector<std::string> others;
};

// Throws std::out_of_range when a setting is missing.
Buildable buildable(const Parameters& parameters, const Game& game, std::size_t colony);

// The production a product costs; 0 for trade goods and housing.
int productCost(const Parameters& parameters, std::string_view product);

// Where prod_format sorts a product of the left list: lower first.
int productOrder(const Parameters& parameters, std::string_view product);

// Ships, spies, housing and trade goods may be in the list more than once;
// a building only once.
bool repeatable(std::string_view product);

// Adds a product at the end of the colony's build list when there is room
// and it may go there; returns whether it did.
bool addToBuildList(Colony& colony, std::string_view product);

} // namespace hegemon
