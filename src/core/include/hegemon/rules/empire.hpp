// SPDX-License-Identifier: GPL-3.0-or-later
//
// The players' empires: their colonies, ships and money, how a game starts
// and what a colony makes in a turn, by the fan patch's rule settings. See
// docs/rules/start.md and docs/rules/economy.md.
#pragma once

#include "hegemon/rules/galaxy.hpp"
#include "hegemon/rules/races.hpp"
#include "hegemon/rules/tech.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

class Parameters;
class Random;

enum class Job : std::uint8_t { Farmer, Worker, Scientist };
inline constexpr std::size_t kJobs = 3;

struct Colony {
    int player = 0;
    int planet = 0;
    std::array<int, kJobs> jobs{}; // how many people do each job
    std::vector<std::string> buildings; // by the names of the fan patch's building table
    int growth = 0; // thousands of people grown towards the next one

    int population() const { return jobs[0] + jobs[1] + jobs[2]; }
    bool has(std::string_view building) const;
};

enum class ShipKind : std::uint8_t { Scout, ColonyShip, OutpostShip };

struct Ship {
    int player = 0;
    ShipKind kind = ShipKind::Scout;
    int star = 0; // where it is
};

struct Player {
    int race = 0; // one of the stock races
    RacePicks picks;
    std::size_t banner = 0;
    bool human = false;
    int money = 0; // BC in the treasury
    Knowledge knowledge;
    int research = 0;   // the field being researched, 0 for none
    int researched = 0; // research points put into it so far
};

struct Game {
    Galaxy galaxy;
    std::vector<Player> players; // the human first
    std::vector<Colony> colonies;
    std::vector<Ship> ships;
    int stardate = 35000; // in tenths of a year
};

// What a colony makes in a turn. Food and money are in halves, as the
// fan patch's settings count them.
struct ColonyOutput {
    int food2 = 0;       // grown by the farmers
    int eaten2 = 0;      // what its people eat
    int production = 0;  // usable, after pollution
    int upkeep = 0;      // production its people need (cybernetic races)
    int research = 0;
    int income2 = 0;     // taxes and trade goods, less the buildings' upkeep
};

// What the galaxy map's side panel shows about an empire.
struct EmpireSummary {
    int money = 0;
    int income = 0;
    int commandPoints = 0;     // the empire has
    int commandUsed = 0;       // its ships need
    int food = 0;              // left over, whole units
    int freighters = 0;
    int freightersUsed = 0;
    int research = 0;      // research points a turn
    int researchField = 0; // the project, 0 for none
    int researchTurns = 0; // about how many turns it takes
};

class Economy {
public:
    // Reads the settings it needs; throws std::out_of_range when the list
    // lacks one.
    explicit Economy(const Parameters& parameters);

    // How the people of a new colony are put to work: as few farmers as feed
    // them, then scientists and workers.
    std::array<int, kJobs> startingJobs(const Player& player, const Planet& planet, int population) const;

    ColonyOutput output(const Game& game, const Colony& colony) const;
    EmpireSummary summary(const Game& game, int player) const;

    // Half food a farmer grows, before the government's bonus.
    int farmerFood2(const Player& player, const Planet& planet) const;
    // Research a scientist makes, in hundredths, with the government.
    int scientistResearch100(const Player& player, const Planet& planet) const;
    // How many of the race's people the planet can hold.
    int maxPopulation(const Player& player, const Planet& planet) const;
    // Thousands of people a colony grows in a turn.
    int growth(const Player& player, const Planet& planet, int population) const;

private:
    int foodTotal2(const Player& player, const Planet& planet, int farmers) const;
    int eaten2(const Player& player, int population) const;
    int production(const Player& player, const Planet& planet, int workers) const;
    int pickValue(const RacePicks& picks, RacePicks::Pick pick, std::string_view name) const;

    const Parameters& p_;
};

// The turns a project takes by the side panel's estimate: until one and a
// half times its cost (measured), at least one.
int researchTurns(int cost, int researched, int perTurn);

// A planet's name: its star's, then "Prime" (`prime`, from HESTRNGS.LBX)
// for the first orbit or a Roman number for the others.
std::string planetName(const Galaxy& galaxy, int planet, const std::string& prime);

// Ends the turn: the stardate moves on a tenth of a year, each colony
// grows, and each player gets the turn's income and research. A field whose research is done is
// learnt (see docs/rules/research.md) and the player has no project then.
void endTurn(Game& game, const Parameters& parameters, const TechTree& tree, Random& random);

// Starts a game in the galaxy: a home colony, ships and money for each
// player (with average technology). `picks` and `banners` are in the order
// of the galaxy's home systems.
Game startGame(const Parameters& parameters, Galaxy galaxy, const std::vector<RacePicks>& picks,
               const std::vector<std::size_t>& banners);

} // namespace hegemon
