// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original's saved games (SAVE1.GAM to SAVE10.GAM) of fan patch 1.51:
// the galaxy, the players, their colonies and ships. See
// docs/formats/savegame.md.
#pragma once

#include "hegemon/rules/empire.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace hegemon {

class Parameters;
class TechTree;

struct SavedGame {
    std::string name; // as the Load Game screen lists it
    Game game;
};

// What the Load Game and Save Game windows show of a saved game.
struct SavegameSummary {
    std::string name;
    int stardate = 0; // in tenths of a year
};

// The size of every saved game of fan patch 1.51.
inline constexpr std::size_t kSavegameSize = 223120;

// Reads a saved game. The rule settings name the buildings, the
// technologies and the map sizes. Throws FormatError when the file is not
// a saved game of this version or is damaged.
SavedGame readSavegame(std::span<const std::uint8_t> bytes, const Parameters& parameters, const TechTree& tree);

// The game in the same layout, under `name` (at most 36 characters).
// Hegemon writes what it keeps of a game; the rest of the file, such as
// leaders and diplomacy, stays zero for now.
std::vector<std::uint8_t> writeSavegame(const Game& game, const std::string& name, const Parameters& parameters);

// Reads only the name and the stardate. Throws FormatError when the file is
// not a saved game of this version.
SavegameSummary readSavegameSummary(std::span<const std::uint8_t> bytes);

} // namespace hegemon
