// SPDX-License-Identifier: GPL-3.0-or-later
//
// What a colony can build: the products of its build list and what they
// cost, read from the fan patch's rule settings. See
// docs/rules/colony-screen.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
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

struct ShipDesign;

// A ship of one of the player's designs: "design1" to "design6", as the fan
// patch's build queue settings name them.
std::string designProduct(std::size_t slot);
// The slot of a design product (0 for "design1"), or none.
std::optional<std::size_t> designSlot(std::string_view product);

// What a colony can add to its build list now, as the build list shows it.
struct Buildable {
    // Trade goods, housing and the buildings, in the order of the fan
    // patch's prod_format (products of the same order still need sorting
    // by their names).
    std::vector<std::string> buildings;
    // Colony bases, freighters, the landing ships and spies.
    std::vector<std::string> others;
    // The player's designs, by slot.
    std::vector<std::string> designs;
};

// Throws std::out_of_range when a setting is missing.
Buildable buildable(const Parameters& parameters, const Game& game, std::size_t colony);

// The production a product costs; 0 for trade goods and housing. A design's
// cost comes from the player's `designs` (0 when it has none in the slot).
int productCost(const Parameters& parameters, std::string_view product, std::span<const ShipDesign> designs = {});

// Where prod_format sorts a product of the left list: lower first.
int productOrder(const Parameters& parameters, std::string_view product);

// Ships, spies, housing and trade goods may be in the list more than once;
// a building only once.
bool repeatable(std::string_view product);

// Adds a product at the end of the colony's build list when there is room
// and it may go there; returns whether it did.
bool addToBuildList(Colony& colony, std::string_view product);

// Buy: what finishing the first of the build list now costs in BC, with
// `produced` of its `cost` made: 10 BC a point in its first tenth, 5 up to
// its half, 2 after.
int buyPrice(int cost, int produced);

// What Buy does for a colony of a player with `money` BC, when it makes
// `perTurn` production a turn.
struct Purchase {
    enum class Kind : std::uint8_t {
        Offer,           // asks whether to spend `price` on `amount`
        Undo,            // takes back this turn's purchase
        NothingSelected, // nothing in the build list
        NotBuyable,      // housing or trade goods
        ThisTurn,        // done this turn anyway
        Short,           // `price` is more than the money
    };
    Kind kind = Kind::NothingSelected;
    int price = 0;
    int amount = 0;
    int shortBy = 0; // for Short: the price less the money
};
Purchase purchase(const Parameters& parameters, const Colony& colony, int money, int perTurn,
                  std::span<const ShipDesign> designs = {});

// Carries out an Offer (pays and fills up the product) or an Undo (gives
// back the BC and takes away the production); does nothing otherwise.
void buy(Colony& colony, int& money, const Purchase& purchase);

} // namespace hegemon
