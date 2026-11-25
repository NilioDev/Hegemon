// SPDX-License-Identifier: GPL-3.0-or-later
//
// What the computer players do each turn, as far as it was seen in the
// original: their research, what their colonies build, where their scouts
// and colony ships go and where the colony ships land. See
// docs/rules/computer-players.md.
#pragma once

#include "hegemon/rules/empire.hpp"

namespace hegemon {

class Parameters;
class Random;
class TechTree;

// The computer players' orders, given before the ships move: a field to
// research when they have none, something to build in each colony with an
// empty build list, a star for each scout and colony ship without one.
void orderComputerPlayers(Game& game, const Parameters& parameters, const TechTree& tree, Random& random);

// After the ships moved: the computer players' colony ships that reached
// the star of the planet they were sent to settle it.
void landComputerColonyShips(Game& game, const Parameters& parameters);

} // namespace hegemon
