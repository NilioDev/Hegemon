// SPDX-License-Identifier: GPL-3.0-or-later
//
// Makes a new galaxy the way the fan patch does: from its map generation
// tables (config.json with the player's settings), so a galaxy follows the
// same chances as one the original makes. See docs/rules/galaxy.md.
#pragma once

#include "hegemon/data/image.hpp"
#include "hegemon/rules/galaxy.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/ui/new_game.hpp"

#include <string>
#include <vector>

namespace hegemon {

class GameData;
class Parameters;
class Random;

struct GalaxyPlayer {
    int race = 0; // one of the stock races
    RacePicks picks;
};

struct GalaxySettings {
    NewGameSettings::GalaxySize size = NewGameSettings::GalaxySize::Medium;
    NewGameSettings::GalaxyAge age = NewGameSettings::GalaxyAge::Average;
    std::vector<GalaxyPlayer> players; // the human first
};

// The names stars get: one home star name per stock race and the list the
// other stars take theirs from (STARNAME.LBX entries 0 and 1).
struct StarNames {
    std::vector<std::string> home;
    std::vector<std::string> other;

    // Throws FormatError when the archive is damaged.
    static StarNames load(GameData& data);
};

// The twelve nebulae at the galaxy map's own scale (STARBG.LBX entries 6,
// 10, ... 50). A star is in a nebula where the picture's colour index is
// above 5. Throws FormatError when the archive is damaged.
std::vector<Canvas> loadNebulae(GameData& data);

class GalaxyGenerator {
public:
    // Without nebula pictures the galaxy has no nebulae.
    GalaxyGenerator(const Parameters& parameters, StarNames names, std::vector<Canvas> nebulae = {});

    // Throws std::runtime_error when the settings leave no room for the
    // stars or the home systems (say, 8 players among 6 stars).
    Galaxy generate(const GalaxySettings& settings, Random& random) const;

private:
    const Parameters& p_;
    StarNames names_;
    std::vector<Canvas> nebulae_;
};

} // namespace hegemon
