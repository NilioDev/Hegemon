// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/savegame.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/rules/tech.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string_view>

namespace hegemon {

namespace {

// Where things are, from the start of the file (see
// docs/formats/savegame.md).
constexpr std::uint32_t kVersion = 0xe0;
constexpr std::size_t kName = 4, kNameLength = 37;
constexpr std::size_t kStardate = 41;

constexpr std::size_t kColonies = 603, kColonySize = 361, kColonySlots = 250;
constexpr std::size_t kPlanetCount = 90855, kPlanets = 90857, kPlanetSize = 17, kPlanetSlots = 360;
constexpr std::size_t kStarCount = 96977, kStars = 96979, kStarSize = 113, kStarSlots = 107;
constexpr std::size_t kPlayers = 109071, kPlayerSize = 3753, kPlayerSlots = 8;
constexpr std::size_t kShipCount = 139094, kShips = 139096, kShipSize = 129, kShipSlots = 500;
constexpr std::size_t kMapWidth = 203753, kMapHeight = 203755;
constexpr std::size_t kNebulae = 203759, kNebulaSize = 5, kNebulaSlots = 4, kNebulaCount = 203779;

// A colony.
constexpr std::size_t kColonyPlayer = 2, kColonyPlanet = 4, kColonyPeople = 12, kColonyPerson = 14;
constexpr std::size_t kColonyPeopleSlots = 42;
constexpr std::size_t kColonyGrowth = 202, kColonyQueue = 279, kColonyBuildings = 312;
constexpr int kNoColony = 0xff;

// A planet.
constexpr std::size_t kPlanetStar = 2, kPlanetOrbit = 3, kPlanetType = 4, kPlanetSize_ = 5, kPlanetGravity = 6,
                      kPlanetClimate = 8, kPlanetImage = 9, kPlanetMinerals = 10, kPlanetSpecial = 15;

// A star.
constexpr std::size_t kStarName = 0, kStarNameLength = 15, kStarX = 15, kStarY = 17, kStarPictureSize = 19,
                      kStarColour = 22, kStarSpecial = 40, kStarWormhole = 41, kStarVisited = 51, kStarOrbits = 74,
                      kStarInNebula = 111;

// A player.
constexpr std::size_t kPlayerName = 0, kPlayerRace = 36, kPlayerBanner = 37, kPlayerHome = 40, kPlayerMoney = 49,
                      kPlayerFields = 195, kPlayerTechs = 278, kPlayerResearched = 490, kPlayerResearch = 800,
                      kPlayerResearchTech = 801, kPlayerDesigns = 805, kPlayerPicks = 2206;
constexpr std::uint8_t kKnown = 3;

// A ship: its design, then where it is.
constexpr std::size_t kDesignSize = 99;
constexpr std::size_t kShipPlayer = 99, kShipStatus = 100, kShipLocation = 101, kShipX = 103, kShipY = 105,
                      kShipSpeed = 108;
constexpr std::uint8_t kOrbiting = 0, kTravelling = 1, kQueued = 6;
constexpr int kTravellingTo = 500;
constexpr int kFirstMonster = 9, kLastMonster = 14;

// What a colony's build list holds, besides buildings by their numbers and
// ships by theirs.
constexpr int kQueueEmpty = -1, kQueueTradeGoods = -2, kQueueHousing = -3, kQueueSpy = -7, kQueueFreighters = -15;
constexpr int kQueueShip = -100;

// A design's type.
constexpr std::uint8_t kTypeColonyShip = 1, kTypeTransport = 2, kTypeOutpost = 4;

constexpr std::array<std::string_view, 4> kMapSizeNames = {"small", "medium", "large", "huge"};

std::string text(std::span<const std::uint8_t> bytes) {
    std::string s;
    for (const std::uint8_t b : bytes) {
        if (b == 0) {
            break;
        }
        s.push_back(static_cast<char>(b));
    }
    return s;
}

class SaveReader {
public:
    SaveReader(std::span<const std::uint8_t> bytes, const Parameters& p, const TechTree& tree)
        : r_(bytes), bytes_(bytes), p_(p), tree_(tree) {}

    SavedGame read() {
        if (bytes_.size() != kSavegameSize || u32(0) != kVersion) {
            throw FormatError("not a saved game of fan patch 1.51");
        }
        SavedGame saved;
        saved.name = text(bytes_.subspan(kName, kNameLength));
        Game& g = saved.game;
        g.stardate = u16(kStardate);
        readGalaxy(g.galaxy);
        readPlayers(g);
        readShips(g);
        readColonies(g);
        return saved;
    }

private:
    std::uint8_t u8(std::size_t at) {
        r_.seek(at);
        return r_.u8();
    }
    std::int8_t i8(std::size_t at) { return static_cast<std::int8_t>(u8(at)); }
    std::uint16_t u16(std::size_t at) {
        r_.seek(at);
        return r_.u16();
    }
    std::int16_t i16(std::size_t at) { return static_cast<std::int16_t>(u16(at)); }
    std::uint32_t u32(std::size_t at) {
        r_.seek(at);
        return r_.u32();
    }
    std::span<const std::uint8_t> span(std::size_t at, std::size_t count) {
        r_.seek(at);
        return r_.bytes(count);
    }

    template <typename E>
    E limited(int value, int last, const char* what) {
        if (value < 0 || value > last) {
            throw FormatError(std::string("saved game: bad ") + what);
        }
        return static_cast<E>(value);
    }

    void readGalaxy(Galaxy& g) {
        g.width = u16(kMapWidth);
        g.height = u16(kMapHeight);
        if (g.width <= 0 || g.height <= 0) {
            throw FormatError("saved game: no map size");
        }
        g.mapSize = 1;
        for (std::size_t m = 0; m < kMapSizeNames.size(); ++m) {
            if (p_.integer("map_generation", {kMapSizeNames[m]}, "width") == g.width) {
                g.mapSize = static_cast<int>(m);
            }
        }

        const std::size_t planets = u16(kPlanetCount), stars = u16(kStarCount);
        if (planets > kPlanetSlots || stars > kStarSlots || stars == 0) {
            throw FormatError("saved game: bad number of stars or planets");
        }
        for (std::size_t i = 0; i < stars; ++i) {
            const std::size_t at = kStars + i * kStarSize;
            Star s;
            s.name = text(span(at + kStarName, kStarNameLength));
            s.x = u16(at + kStarX);
            s.y = u16(at + kStarY);
            s.size = std::min<int>(u8(at + kStarPictureSize), 2);
            s.colour = limited<StarColour>(u8(at + kStarColour), static_cast<int>(StarColour::BlackHole), "star colour");
            s.special = limited<Special>(u8(at + kStarSpecial), static_cast<int>(Special::Orion), "star special");
            s.wormhole = i8(at + kStarWormhole);
            if (s.wormhole >= static_cast<int>(stars)) {
                throw FormatError("saved game: bad wormhole");
            }
            s.inNebula = u8(at + kStarInNebula) != 0;
            for (std::size_t o = 0; o < kOrbits; ++o) {
                const int planet = i16(at + kStarOrbits + 2 * o);
                if (planet >= static_cast<int>(planets)) {
                    throw FormatError("saved game: bad orbit");
                }
                s.orbits[o] = planet < 0 ? kNoPlanet : planet;
            }
            visited_.push_back(u8(at + kStarVisited));
            g.stars.push_back(std::move(s));
        }
        for (std::size_t i = 0; i < planets; ++i) {
            const std::size_t at = kPlanets + i * kPlanetSize;
            Planet q;
            q.star = u8(at + kPlanetStar);
            if (q.star >= static_cast<int>(stars)) {
                throw FormatError("saved game: bad planet star");
            }
            q.orbit = limited<int>(u8(at + kPlanetOrbit), static_cast<int>(kOrbits) - 1, "orbit");
            q.type = limited<SatelliteType>(u8(at + kPlanetType), static_cast<int>(SatelliteType::Planet), "satellite");
            if (static_cast<int>(q.type) == 0) {
                throw FormatError("saved game: bad satellite");
            }
            q.size = limited<PlanetSize>(u8(at + kPlanetSize_), static_cast<int>(PlanetSize::Huge), "planet size");
            q.gravity = limited<Gravity>(u8(at + kPlanetGravity), static_cast<int>(Gravity::Heavy), "gravity");
            q.climate = limited<Climate>(u8(at + kPlanetClimate), static_cast<int>(Climate::Gaia), "climate");
            q.image = limited<int>(u8(at + kPlanetImage), kPlanetImages - 1, "planet picture");
            q.minerals = limited<Minerals>(u8(at + kPlanetMinerals), static_cast<int>(Minerals::UltraRich), "minerals");
            q.special = limited<Special>(u8(at + kPlanetSpecial), static_cast<int>(Special::Orion), "planet special");
            g.planets.push_back(q);
        }
        const std::size_t nebulae = std::min<std::size_t>(u8(kNebulaCount), kNebulaSlots);
        for (std::size_t i = 0; i < nebulae; ++i) {
            const std::size_t at = kNebulae + i * kNebulaSize;
            g.nebulae.push_back({u8(at + 4), u16(at), u16(at + 2)});
        }
    }

    void readPlayers(Game& g) {
        const std::size_t stars = g.galaxy.stars.size();
        for (std::size_t i = 0; i < kPlayerSlots; ++i) {
            const std::size_t at = kPlayers + i * kPlayerSize;
            if (text(span(at + kPlayerName, 20)).empty()) {
                break;
            }
            Player pl;
            pl.race = limited<int>(u8(at + kPlayerRace), 12, "race");
            pl.banner = limited<std::size_t>(u8(at + kPlayerBanner), 7, "banner");
            pl.human = i == 0;
            pl.money = static_cast<int>(std::min<std::uint32_t>(u32(at + kPlayerMoney), 1'000'000'000));
            const auto picks = span(at + kPlayerPicks, RacePicks::Count);
            for (std::size_t k = 0; k < RacePicks::Count; ++k) {
                pl.picks.values[k] = static_cast<std::int8_t>(picks[k]);
            }
            pl.knowledge.fields.resize(tree_.fieldCount());
            for (std::size_t f = 0; f < tree_.fieldCount(); ++f) {
                pl.knowledge.fields[f] = u8(at + kPlayerFields + f) == kKnown;
            }
            pl.knowledge.techs.resize(tree_.techCount());
            for (std::size_t t = 1; t < tree_.techCount(); ++t) {
                pl.knowledge.techs[t] = u8(at + kPlayerTechs + t) == kKnown;
            }
            pl.research = u8(at + kPlayerResearch);
            if (static_cast<std::size_t>(pl.research) >= tree_.fieldCount()) {
                pl.research = 0;
            }
            pl.researchTech = pl.research == 0 ? 0 : u8(at + kPlayerResearchTech);
            pl.researched = u16(at + kPlayerResearched);
            for (std::size_t s = 0; s < kDesignSlots; ++s) {
                pl.designs.push_back(design(at + kPlayerDesigns + s * kDesignSize));
            }
            pl.explored.resize(stars);
            for (std::size_t s = 0; s < stars; ++s) {
                // Every player has "visited" the black holes; the
                // original names none of them on the map.
                pl.explored[s] = ((visited_[s] >> i) & 1) != 0 &&
                                 g.galaxy.stars[s].colour != StarColour::BlackHole;
            }
            const int home = u16(at + kPlayerHome);
            if (home >= static_cast<int>(g.galaxy.planets.size())) {
                throw FormatError("saved game: bad home planet");
            }
            g.galaxy.homes.push_back({pl.race, g.galaxy.planets[static_cast<std::size_t>(home)].star, home});
            g.players.push_back(std::move(pl));
        }
        if (g.players.empty()) {
            throw FormatError("saved game: no players");
        }
    }

    ShipDesign design(std::size_t at) {
        ShipDesign d;
        d.name = text(span(at, 16));
        d.hull = limited<Hull>(u8(at + 16), static_cast<int>(Hull::DoomStar), "hull");
        d.shield = u8(at + 18);
        d.drive = u8(at + 19);
        d.computer = u8(at + 21);
        d.armor = u8(at + 22);
        const auto specials = span(at + 23, 5);
        for (std::size_t b = 0; b < specials.size(); ++b) {
            d.specials |= static_cast<std::uint64_t>(specials[b]) << (8 * b);
        }
        for (std::size_t w = 0; w < 8; ++w) {
            const std::size_t m = at + 28 + 8 * w;
            WeaponMount mount;
            mount.weapon = u16(m);
            if (mount.weapon == 0) {
                continue;
            }
            mount.count = u8(m + 2);
            mount.arc = u8(m + 4);
            mount.mods = u16(m + 5);
            mount.ammo = u8(m + 7);
            d.weapons.push_back(mount);
        }
        d.image = u8(at + 92);
        d.nameIndex = d.image;
        d.cost = u16(at + 94);
        d.combatSpeed = u8(at + 96);
        return d;
    }

    // A ship's kind and design slot, from its design and its player's.
    void kindOf(const Game& g, std::size_t at, Ship& s) {
        const std::uint8_t type = u8(at + 17);
        if (type == kTypeColonyShip) {
            s.kind = ShipKind::ColonyShip;
            return;
        }
        if (type == kTypeTransport) {
            s.kind = ShipKind::Transport;
            return;
        }
        if (type == kTypeOutpost) {
            s.kind = ShipKind::OutpostShip;
            return;
        }
        s.kind = ShipKind::Combat;
        if (s.player < 0 || static_cast<std::size_t>(s.player) >= g.players.size()) {
            return;
        }
        const std::string name = text(span(at, 16));
        const auto& designs = g.players[static_cast<std::size_t>(s.player)].designs;
        for (std::size_t k = 0; k < designs.size(); ++k) {
            if (designs[k].name == name && static_cast<int>(designs[k].hull) == u8(at + 16)) {
                s.design = static_cast<int>(k);
                break;
            }
        }
        if (s.design == 0) {
            s.kind = ShipKind::Scout;
            s.design = -1;
        }
    }

    void readShips(Game& g) {
        const std::size_t count = u16(kShipCount);
        if (count > kShipSlots) {
            throw FormatError("saved game: bad number of ships");
        }
        const int stars = static_cast<int>(g.galaxy.stars.size());
        for (std::size_t i = 0; i < count; ++i) {
            const std::size_t at = kShips + i * kShipSize;
            const int player = u8(at + kShipPlayer);
            const std::uint8_t status = u8(at + kShipStatus);
            const int location = u16(at + kShipLocation);
            if (player >= kFirstMonster && player <= kLastMonster) {
                if (location < stars) {
                    g.galaxy.monsters.push_back({static_cast<MonsterKind>(player - kFirstMonster), location});
                }
                continue;
            }
            if (player >= static_cast<int>(g.players.size())) {
                continue;
            }
            Ship s;
            s.player = player;
            kindOf(g, at, s);
            if (status == kQueued) {
                queued_[i] = s.kind == ShipKind::ColonyShip    ? std::string(kColonyShip)
                             : s.kind == ShipKind::OutpostShip ? std::string(kOutpostShip)
                             : s.kind == ShipKind::Transport   ? std::string(kTransportShip)
                             : s.kind == ShipKind::Scout       ? designProduct(0)
                             : s.design >= 0                   ? designProduct(static_cast<std::size_t>(s.design))
                                                               : std::string();
                continue;
            }
            if (status == kOrbiting && location < stars) {
                s.star = location;
                const Star& at_ = g.galaxy.stars[static_cast<std::size_t>(location)];
                s.x = at_.x;
                s.y = at_.y;
            } else if (status == kTravelling && location >= kTravellingTo && location - kTravellingTo < stars) {
                s.star = -1;
                s.destination = location - kTravellingTo;
                s.x = u16(at + kShipX);
                s.y = u16(at + kShipY);
                s.speed = u8(at + kShipSpeed);
            } else {
                continue; // not understood yet
            }
            g.ships.push_back(s);
        }
    }

    void readColonies(Game& g) {
        std::map<int, std::string> buildings;
        const ParameterDefinition* table = p_.definition("building");
        if (table != nullptr && !table->rowNames.empty()) {
            for (const std::string& name : table->rowNames[0]) {
                buildings[static_cast<int>(p_.integer("building", {name}, "id"))] = name;
            }
        }
        for (std::size_t i = 0; i < kColonySlots; ++i) {
            const std::size_t at = kColonies + i * kColonySize;
            const int player = u8(at + kColonyPlayer);
            if (player == kNoColony) {
                continue;
            }
            const int planet = u16(at + kColonyPlanet);
            if (player >= static_cast<int>(g.players.size()) || planet >= static_cast<int>(g.galaxy.planets.size())) {
                continue;
            }
            Colony c;
            c.player = player;
            c.planet = planet;
            const std::size_t people = std::min<std::size_t>(u8(at + kColonyPeople), kColonyPeopleSlots);
            for (std::size_t k = 0; k < people; ++k) {
                const std::uint32_t person = u32(at + kColonyPerson + 4 * k);
                const std::size_t job = (person >> 7) & 3;
                if (job < kJobs) {
                    ++c.jobs[job];
                }
            }
            c.growth = u16(at + kColonyGrowth);
            for (const auto& [id, name] : buildings) {
                if (id > 0 && kColonyBuildings + static_cast<std::size_t>(id) < kColonySize &&
                    u8(at + kColonyBuildings + static_cast<std::size_t>(id)) != 0) {
                    c.buildings.push_back(name);
                }
            }
            for (std::size_t k = 0; k < Colony::kBuildListLength; ++k) {
                const int code = i16(at + kColonyQueue + 2 * k);
                std::string product;
                if (code > 0) {
                    const auto b = buildings.find(code);
                    product = b != buildings.end() ? b->second : std::string();
                } else if (code == kQueueTradeGoods) {
                    product = kTradeGoods;
                } else if (code == kQueueHousing) {
                    product = kHousing;
                } else if (code == kQueueSpy) {
                    product = kSpy;
                } else if (code == kQueueFreighters) {
                    product = kFreighters;
                } else if (code <= kQueueShip) {
                    const auto q = queued_.find(static_cast<std::size_t>(kQueueShip - code));
                    product = q != queued_.end() ? q->second : std::string();
                } else if (code == kQueueEmpty) {
                    break;
                }
                if (!product.empty()) {
                    c.buildList.push_back(product);
                }
            }
            // Measured with no colony screen opened yet; the original shows
            // a colony with nothing to build at the end of the turn.
            c.idleShown = !c.buildList.empty();
            g.colonies.push_back(std::move(c));
        }
    }

    Reader r_;
    std::span<const std::uint8_t> bytes_;
    const Parameters& p_;
    const TechTree& tree_;
    std::vector<std::uint8_t> visited_;
    std::map<std::size_t, std::string> queued_;
};

} // namespace

SavedGame readSavegame(std::span<const std::uint8_t> bytes, const Parameters& parameters, const TechTree& tree) {
    return SaveReader(bytes, parameters, tree).read();
}

} // namespace hegemon
