// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/colony_screen.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/production.hpp"
#include "hegemon/ui/build_list.hpp"

#include <algorithm>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colony-screen.md).
constexpr const char* kSkyArchive = "COLONY2.LBX";
constexpr std::size_t kSkyEntry = 49;
constexpr const char* kLandscapeArchive = "PLANETS.LBX";
constexpr const char* kFrameArchive = "COLPUPS.LBX";
constexpr std::size_t kFrameEntry = 5;
// HESTRNGS.LBX: the word for a star's first planet.
constexpr std::size_t kPrimeText = 160;
constexpr std::size_t kTurnsText = 226; // "%d turns", also for one
// The box of what is built, measured: its name in font 1 centred on 578 at
// y 38, the turns from (531, 103) and the progress, "made / cost", ending
// at 626, in font 0; one pixel between letters, shades 245 and 253 and a
// black shadow.
constexpr int kProductCentre = 578, kProductY = 38, kTurnsX = 531, kBoxTextY = 103, kProgressRight = 626;
constexpr std::size_t kProductFont = 1, kBoxFont = 0;
// ESTRINGS.LBX messages.
constexpr std::size_t kTitleMessage = 97;      // "%sColony of %s"
constexpr std::size_t kPopulationMessage = 424; // "Pop %d,%03d k (%+dk)"
// What Buy says.
constexpr std::size_t kBuyMessage = 269;        // "Do you want to spend %d BC to buy %d production?"
constexpr std::size_t kShortMessage = 564;      // "... You are short by %d BC."
constexpr std::size_t kNotBuyableMessage = 568; // "You cannot buy Housing or Trade Goods..."
constexpr std::size_t kThisTurnMessage = 578;   // "... faster than in a single turn."
constexpr std::size_t kNothingMessage = 587;    // "You have no product selected to buy."
// The title in font 4, centred on 320 at the top; the population in font
// 2 ending at 639, three pixels down. Two pixels between letters; shades
// in colours 245, 253 and 252.
constexpr std::size_t kTitleFont = 4, kPopulationFont = 2;
constexpr int kTitleCentre = 320, kTitleY = 0;
constexpr int kPopulationRight = 639, kPopulationY = 3;
constexpr int kSpacing = 2;
constexpr std::uint8_t kShade1 = 245, kShade2 = 253, kShade3 = 252;
// The rows of icons (COLONY2.LBX): one for a unit, another for ten, and
// darker ones for what is missing (assumed).
constexpr const char* kIconArchive = kSkyArchive;
constexpr int kFood = 0, kProduction = 1, kResearch = 2, kMoney = 3;
constexpr int kTen = 4; // the entry for ten is four further on
constexpr int kMissingFood = 8, kPollution = 9, kMissingMoney = 11, kMissingFood10 = 12, kPollution10 = 13,
              kMissingMoney10 = 15;
constexpr int kGovernmentIcon = 19; // and on by the government pick
constexpr int kIcons = 27;
// Measured: the rows start at x 128 and fill at most 150 pixels, 20 apart
// when they fit.
constexpr int kRowX = 128, kRowWidth = 150, kMaxStep = 20;
constexpr std::array<int, ColonyScreen::kRows> kRowY = {32, 64, 94, 124};
constexpr int kGovernmentX = 310, kGovernmentY = 32;
// The people (RACEICON.LBX, thirteen entries a race): a farmer, a worker and
// a scientist, 30 pixels apart in their rows.
constexpr const char* kPeopleArchive = "RACEICON.LBX";
constexpr std::size_t kPeoplePerRace = 13;
constexpr std::array<std::size_t, 3> kPersonEntry = {1, 3, 5};
constexpr int kPeopleX = 310, kPersonStep = 30, kPeopleWidth = 180;
constexpr std::array<int, 3> kPeopleY = {62, 92, 122};
// A job row takes the people from 2 pixels above them down to the next row
// and to the panel's right side (assumed from the panel's lines).
constexpr int kRowTop = -2, kRowHeight = 30, kRowRight = 515;
// People being moved are drawn beside the pointer, 15 pixels apart, in the
// job of the row under it (seen; the distances are assumed).
constexpr int kHeldDX = 8, kHeldDY = -10, kHeldStep = 15;

// `amount` in icons of ten first, then units.
void addIcons(std::vector<int>& row, int amount, int unit, int ten) {
    for (; amount >= 10; amount -= 10) {
        row.push_back(ten);
    }
    for (; amount > 0; --amount) {
        row.push_back(unit);
    }
}

// The planets of the system on the left (COLSYSDI.LBX), measured: an
// arrow at x 6 for each orbit (frame 3 for the colony shown), the planet
// centred on (22, 38) and on 24 pixels down an orbit, its text in font 1 at
// x 35, a second line 11 pixels down.
constexpr const char* kSystemArchive = "COLSYSDI.LBX";
constexpr std::size_t kFirstPlanetIcon = 6; // + 5 × (climate + 1) + size
constexpr std::size_t kGasGiantIcon = 61, kAsteroidsIcon = 63, kArrowIcon = 64;
// The arrow by the colony shown pulses through its frames, one a tick
// (how fast is assumed; the original's pictures caught frames 1 to 5).
constexpr int kArrowX = 6, kListY = 31, kOrbitStep = 24, kIconCentreX = 22, kIconCentreY = 38;
constexpr int kListX = 35, kSecondLine = 11, kListSpacing = 1;
constexpr std::size_t kListFont = 1;
// Wider texts are broken at a space (assumed).
constexpr int kListWidth = 60;
constexpr std::uint8_t kShownShade1 = 236, kShownShade2 = 238;
// ESTRINGS.LBX messages.
// Two lines each: "%s %s- %s\n%s%s" with the size, the climate, the
// gravity, then the minerals (the last blank is left empty); "%s - %s\n%s%s"
// with the race, the gravity, then the minerals and the people.
constexpr std::size_t kPlanetMessage = 44;
constexpr std::size_t kColonyMessage = 41;
constexpr std::size_t kPeopleMessage = 8;    // " (%d/%d)"
constexpr std::size_t kGasGiantMessage = 306, kAsteroidsMessage = 186, kRaceMessage = 645;
constexpr std::array<std::size_t, 5> kSizeMessages = {502, 604, 605, 606, 607};
constexpr std::array<std::size_t, 10> kClimateMessages = {539, 719, 720, 721, 722, 399, 501, 184, 723, 303};
// Ultra poor and ultra rich are shortened; abundant says nothing.
constexpr std::array<std::size_t, 5> kMineralMessages = {546, 423, 0, 450, 547};
constexpr std::size_t kLowGravityMessage = 361, kHeavyGravityMessage = 603;
constexpr std::size_t kLastMessage = 723;

// The landscape's buildings: BLDG0.LBX to BLDG4.LBX, 360 entries each
// (BLDG4 324), counted on from one file to the next: 36 places for each
// picture, the pictures in the order of the fan patch's building table.
// Place 0 is at the front, 35 at the back. Roads are COLROADS.LBX entries.
constexpr std::size_t kPlaces = 36, kPicturesPerFile = 10;
constexpr int kCapitolPicture = 8, kMarinesPicture = 21;
// Pictures of no building the colony has, drawn as part of the layout.
constexpr int kDomes = 2, kModules = 13;
// Measured: colours 240 and 241 are shadow.
constexpr std::uint8_t kShadow = 240, kShadow2 = 241;
struct Placed {
    int picture;
    int place;
};
struct Layout {
    std::vector<Placed> pieces;
    std::vector<int> roads;
};
// Measured on 39 new games: with a capitol one of four layouts, about as
// often each; without one (Unification) three were seen.
const std::array<Layout, ColonyScreen::kLayouts> kCapitolLayouts = {{
    {{{kCapitolPicture, 10}, {kMarinesPicture, 7}, {kModules, 17}, {kModules, 26}, {kModules, 28}}, {16, 25, 28, 31, 32, 50, 72}},
    {{{kDomes, 1}, {kCapitolPicture, 15}, {kModules, 0}, {kModules, 20}, {kMarinesPicture, 28}},
     {6, 20, 26, 34, 42, 43, 49, 50, 66, 67, 74}},
    {{{kDomes, 6}, {kCapitolPicture, 10}, {kModules, 0}, {kModules, 35}, {kMarinesPicture, 33}},
     {0, 5, 6, 23, 37, 43, 48, 50, 51, 61, 62, 78}},
    {{{kDomes, 24}, {kCapitolPicture, 10}, {kModules, 0}, {kModules, 25}, {kMarinesPicture, 14}},
     {0, 6, 7, 10, 13, 20, 43, 46, 51, 53, 54, 58, 59}},
}};
const std::array<Layout, 3> kUnifiedLayouts = {{
    {{{kModules, 0}, {kModules, 18}, {kModules, 28}, {kMarinesPicture, 7}}, {0, 25, 33, 34, 39, 74}},
    {{{kDomes, 0}, {kModules, 10}, {kModules, 14}, {kMarinesPicture, 6}}, {7, 13, 14, 42, 51, 59, 78, 79}},
    {{{kDomes, 28}, {kModules, 0}, {kModules, 14}, {kMarinesPicture, 15}}, {14, 20, 28, 42, 59, 66}},
}};

Image buildingPicture(GameData& data, int picture, int place) {
    const std::size_t at = static_cast<std::size_t>(picture) * kPlaces + static_cast<std::size_t>(place);
    const std::string file = "BLDG" + std::to_string(at / (kPlaces * kPicturesPerFile)) + ".LBX";
    return Image::parse(data.archive(file).entry(at % (kPlaces * kPicturesPerFile)));
}

TextStyle listStyle(bool shown) {
    TextStyle style;
    style.shades[1] = shown ? kShownShade1 : kShade1;
    style.shades[2] = style.shades[3] = shown ? kShownShade2 : kShade2;
    style.spacing = kListSpacing;
    return style;
}

void addGap(std::vector<int>& row) {
    if (!row.empty()) {
        row.push_back(ColonyScreen::kGap);
    }
}

} // namespace

ColonyScreen::ColonyScreen(GameData& data, const Game& game, const Economy& economy, std::size_t colony)
    : sky_(Image::parse(data.archive(kSkyArchive).entry(kSkyEntry))),
      frame_(Image::parse(data.archive(kFrameArchive).entry(kFrameEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const Colony& c = game.colonies.at(colony);
    const Planet& planet = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
    const Player& player = game.players.at(static_cast<std::size_t>(c.player));
    // Three landscapes for each climate, in the order of the climates.
    const auto landscape = static_cast<std::size_t>(planet.climate) * static_cast<std::size_t>(kPlanetImages) +
                           static_cast<std::size_t>(std::clamp(planet.image, 0, kPlanetImages - 1));
    landscape_ = Image::parse(data.archive(kLandscapeArchive).entry(landscape));
    for (int i = 0; i < kIcons; ++i) {
        icons_.push_back(Image::parse(data.archive(kIconArchive).entry(static_cast<std::size_t>(i))));
    }
    const auto race = static_cast<std::size_t>(player.race);
    for (std::size_t j = 0; j < people_.size(); ++j) {
        people_[j] = Image::parse(data.archive(kPeopleArchive).entry(race * kPeoplePerRace + kPersonEntry[j]));
    }
    jobs_ = c.jobs;
    const bool capitol = c.has("capitol");
    const auto layoutIndex = static_cast<std::size_t>(std::max(0, c.layout));
    const Layout& layout = capitol ? kCapitolLayouts[layoutIndex % kCapitolLayouts.size()]
                                   : kUnifiedLayouts[layoutIndex % kUnifiedLayouts.size()];
    for (const int road : layout.roads) {
        roads_.push_back(Image::parse(data.archive("COLROADS.LBX").entry(static_cast<std::size_t>(road))));
    }
    std::vector<Placed> placed = layout.pieces;
    std::sort(placed.begin(), placed.end(), [](const Placed& a, const Placed& b) { return a.place > b.place; });
    for (const Placed& p : placed) {
        if (p.picture == kMarinesPicture && !c.has("marine_barracks")) {
            continue;
        }
        pieces_.push_back({p.place, buildingPicture(data, p.picture, p.place)});
    }
    government_ = std::clamp(static_cast<int>(player.picks[RacePicks::Government]), 0, kIcons - 1 - kGovernmentIcon);

    // Measured over the thirteen races' home colonies; see
    // docs/rules/colony-screen.md.
    const ColonyOutput out = economy.output(game, c);
    auto& money = rows_[Money];
    addIcons(money, out.maintenance2 / 2, kMoney, kMoney + kTen);
    addGap(money);
    // Halves round up, as on the galaxy map.
    if (out.income2 >= 0) {
        addIcons(money, (out.income2 + 1) / 2, kMoney, kMoney + kTen);
    } else {
        addIcons(money, -out.income2 / 2, kMissingMoney, kMissingMoney10);
    }
    if (!money.empty() && money.back() == kGap) {
        money.pop_back();
    }
    auto& food = rows_[Food];
    const int grown = out.food2 / 2, eaten = out.eaten2 / 2;
    addIcons(food, std::min(grown, eaten), kFood, kFood + kTen);
    addIcons(food, eaten - std::min(grown, eaten), kMissingFood, kMissingFood10);
    if (out.upkeep > 0) {
        // A cybernetic race eats production too; a place is left after it
        // even when nothing is left over (measured on the Meklars).
        addIcons(food, out.upkeep, kProduction, kProduction + kTen);
        food.push_back(kGap);
    }
    if (grown > eaten) {
        if (food.empty() || food.back() != kGap) {
            addGap(food);
        }
        addIcons(food, grown - eaten, kFood, kFood + kTen);
    }
    auto& production = rows_[Production];
    addIcons(production, std::max(0, out.production - out.upkeep), kProduction, kProduction + kTen);
    addIcons(production, out.pollution, kPollution, kPollution10);
    addIcons(rows_[Research], out.research, kResearch, kResearch + kTen);
    const auto colours = data.archive("FONTS.LBX").entry(kPointerEntry);
    if (colours.size() < 1024) {
        throw FormatError("FONTS.LBX: the colony screen's colours are missing");
    }
    base_.set(0, colours.subspan(0, 1024));
    {
        // Measured: a building's shadow (colours 240 and 241) darkens what is under
        // it to the colour nearest half as bright, counted in the palette's
        // six bits a channel; the lowest index wins a tie.
        Palette p = base_;
        sky_.applyPalette(p);
        landscape_.applyPalette(p);
        frame_.applyPalette(p);
        const auto six = [&](std::size_t i, int channel) {
            const Color& col = p.colors[i];
            const int v = channel == 0 ? col.r : channel == 1 ? col.g : col.b;
            return (v * 63 + 127) / 255;
        };
        for (std::size_t i = 0; i < 256; ++i) {
            int best = -1;
            for (std::size_t j = 0; j < 256; ++j) {
                int d = 0;
                for (int ch = 0; ch < 3; ++ch) {
                    const int e = six(j, ch) - six(i, ch) / 2;
                    d += e * e;
                }
                if (best < 0 || d < best) {
                    best = d;
                    shade_[i] = static_cast<std::uint8_t>(j);
                }
            }
        }
    }

    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings.size() <= kPrimeText) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    if (messages.size() <= kLastMessage) {
        throw FormatError("too few messages in ESTRINGS.LBX");
    }
    title_ = formatMessage(messages[kTitleMessage],
                           {std::string(), planetName(game.galaxy, c.planet, strings[kPrimeText])});
    planetList(data, game, economy, c, messages);
    buyQuestion_ = messages[kBuyMessage];
    buyShort_ = messages[kShortMessage];
    buyNotBuyable_ = messages[kNotBuyableMessage];
    buyThisTurn_ = messages[kThisTurnMessage];
    buyNothing_ = messages[kNothingMessage];
    if (strings.size() <= kTurnsText) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    {
        // Measured: with nothing to build "1 turns" and "0 / 0".
        int cost = 0, turns = 1;
        if (!c.buildList.empty()) {
            const std::string& what = c.buildList.front();
            product_ = productName(data, what);
            picture_.emplace(data, what, game.players.at(static_cast<std::size_t>(c.player)).race);
            cost = productCost(data.parameters(), what);
            const ColonyOutput made = economy.output(game, c);
            const int perTurn = std::max(0, made.production - made.upkeep);
            const int left = std::max(0, cost - c.produced);
            // Assumed: with nothing made a turn, as if one were made.
            turns = std::max(1, (left + std::max(1, perTurn) - 1) / std::max(1, perTurn));
        }
        turns_ = formatMessage(strings[kTurnsText], {static_cast<long long>(turns)});
        progress_ = std::to_string(cost == 0 ? 0 : c.produced) + " / " + std::to_string(cost);
    }
    population_ = formatMessage(messages[kPopulationMessage],
                                {static_cast<long long>(c.population()), static_cast<long long>(c.growth),
                                 static_cast<long long>(economy.growth(game, c))});

    // The sky shows where the landscape leaves it; the buildings don't
    // change while the screen is open, so the scene is drawn once.
    sky_.draw(0, scene_);
    landscape_.draw(0, scene_);
    for (const Image& road : roads_) {
        drawShaded(road, scene_);
    }
    for (const Piece& piece : pieces_) {
        drawShaded(piece.picture, scene_);
    }
    roads_.clear();
    pieces_.clear();
}

void ColonyScreen::planetList(GameData& data, const Game& game, const Economy& economy, const Colony& shown,
                              const std::vector<std::string>& messages) {
    const Planet& home = game.galaxy.planets.at(static_cast<std::size_t>(shown.planet));
    const Star& star = game.galaxy.stars.at(static_cast<std::size_t>(home.star));
    arrow_ = Image::parse(data.archive(kSystemArchive).entry(kArrowIcon));
    const Font& font = fonts_.font(kListFont);
    for (std::size_t k = 0; k < kOrbits; ++k) {
        Orbit& o = orbits_[k];
        const int index = star.orbits[k];
        if (index < 0 || static_cast<std::size_t>(index) >= game.galaxy.planets.size()) {
            continue;
        }
        const Planet& planet = game.galaxy.planets[static_cast<std::size_t>(index)];
        o.shown = index == shown.planet;
        std::size_t icon = kAsteroidsIcon;
        std::string text;
        const std::string gravity = planet.gravity == Gravity::Low     ? messages[kLowGravityMessage]
                                    : planet.gravity == Gravity::Heavy ? messages[kHeavyGravityMessage]
                                                                       : std::string();
        if (planet.type == SatelliteType::AsteroidBelt) {
            text = messages[kAsteroidsMessage];
        } else if (planet.type == SatelliteType::GasGiant) {
            icon = kGasGiantIcon;
            text = messages[kGasGiantMessage];
        } else {
            const auto size = static_cast<std::size_t>(planet.size) % kSizeMessages.size();
            const auto climate = static_cast<std::size_t>(planet.climate) % kClimateMessages.size();
            icon = kFirstPlanetIcon + 5 * (climate + 1) + size;
            const std::size_t mineralsMessage = kMineralMessages[static_cast<std::size_t>(planet.minerals) % 5];
            const std::string minerals = mineralsMessage == 0 ? std::string() : messages[mineralsMessage];
            const Colony* colony = nullptr;
            for (const Colony& c : game.colonies) {
                if (c.planet == index) {
                    colony = &c;
                }
            }
            if (colony != nullptr) {
                const Player& owner = game.players.at(static_cast<std::size_t>(colony->player));
                const std::string people = formatMessage(messages[kPeopleMessage],
                                                         {static_cast<long long>(colony->population()),
                                                          static_cast<long long>(economy.maxPopulation(owner, planet))});
                text = formatMessage(messages[kColonyMessage],
                                     {messages[kRaceMessage + static_cast<std::size_t>(owner.race) % 13], gravity,
                                      minerals, people});
            } else {
                text = formatMessage(messages[kPlanetMessage],
                                     {messages[kSizeMessages[size]], messages[kClimateMessages[climate]], gravity,
                                      minerals, std::string()});
            }
        }
        if (const auto newline = text.find('\n'); newline != std::string::npos) {
            o.lines[0] = text.substr(0, newline);
            o.lines[1] = text.substr(newline + 1);
        } else {
            // One line, or two broken at the last space that fits.
            o.lines[0] = text;
            for (std::size_t at = text.size(); font.textWidth(o.lines[0], kListSpacing) > kListWidth;) {
                at = text.rfind(' ', at - 1);
                if (at == std::string::npos || at == 0) {
                    o.lines[0] = text;
                    o.lines[1].clear();
                    break;
                }
                o.lines[0] = text.substr(0, at);
                o.lines[1] = text.substr(at + 1);
            }
        }
        o.icon = Image::parse(data.archive(kSystemArchive).entry(icon));
        o.planet = true;
    }
}

int ColonyScreen::iconStep(std::size_t places) {
    if (places < 2) {
        return kMaxStep;
    }
    return std::min(kMaxStep, kRowWidth / static_cast<int>(places - 1));
}

void ColonyScreen::tick(std::uint64_t ms) {
    const std::uint64_t ticks = timer_.advance(ms);
    if (arrow_.frameCount() > 0) {
        arrowFrame_ = static_cast<std::size_t>((arrowFrame_ + ticks) % arrow_.frameCount());
    }
}

std::string ColonyScreen::buyText(const Purchase& p) const {
    switch (p.kind) {
    case Purchase::Kind::Offer:
        return formatMessage(buyQuestion_, {static_cast<long long>(p.price), static_cast<long long>(p.amount)});
    case Purchase::Kind::Short:
        return formatMessage(buyShort_, {static_cast<long long>(p.shortBy)});
    case Purchase::Kind::NotBuyable:
        return buyNotBuyable_;
    case Purchase::Kind::ThisTurn:
        return buyThisTurn_;
    case Purchase::Kind::NothingSelected:
        return buyNothing_;
    case Purchase::Kind::Undo:
        break;
    }
    return {};
}

namespace {

int personStep(int n) {
    return n < 2 ? kPersonStep : std::min(kPersonStep, kPeopleWidth / (n - 1));
}

} // namespace

std::size_t ColonyScreen::rowAt(int y, bool& inside) const {
    for (std::size_t j = 0; j < kPeopleY.size(); ++j) {
        const int top = kPeopleY[j] + kRowTop;
        if (y >= top && y < top + kRowHeight) {
            inside = true;
            return j;
        }
    }
    inside = false;
    return 0;
}

int ColonyScreen::personAt(int x, int y, std::size_t& job) const {
    bool inside = false;
    job = rowAt(y, inside);
    if (!inside || x < kPeopleX || x >= kRowRight) {
        return -1;
    }
    const int n = jobs_[job];
    const int step = personStep(n);
    const int k = (x - kPeopleX) / step;
    return k < n ? k : -1;
}

void ColonyScreen::pointerMoved(int x, int y) {
    pointerX_ = x;
    pointerY_ = y;
}

ColonyScreen::Action ColonyScreen::drop(std::size_t row) {
    const std::size_t from = heldJob_;
    const int count = heldCount_;
    heldCount_ = 0;
    keyboardHold_ = false;
    if (row == from || count <= 0) {
        return Action::None;
    }
    jobs_[from] -= count;
    jobs_[row] += count;
    return Action::Jobs;
}

void ColonyScreen::moveCursor(int step) {
    const int n = static_cast<int>(kPeopleY.size());
    cursor_ = static_cast<std::size_t>(((static_cast<int>(cursor_) + step) % n + n) % n);
}

ColonyScreen::Action ColonyScreen::pickOrDrop() {
    if (heldCount_ > 0) {
        return drop(cursor_);
    }
    if (jobs_[cursor_] > 0) {
        heldJob_ = cursor_;
        heldCount_ = 1;
        keyboardHold_ = true;
    }
    return Action::None;
}

std::pair<int, int> ColonyScreen::cursorPoint() const {
    int n = jobs_[cursor_];
    if (heldCount_ > 0 && heldJob_ == cursor_) {
        n -= heldCount_;
    }
    const int k = std::max(0, n - 1);
    return {kPeopleX + personStep(n) * k + kPersonStep / 2, kPeopleY[cursor_] + kPersonStep / 2};
}

void ColonyScreen::press(int x, int y) {
    held_ = Rect{};
    pointerMoved(x, y);
    std::size_t job = 0;
    const int k = personAt(x, y, job);
    if (k >= 0) {
        // Measured: the person pressed is picked up with all to its right.
        heldJob_ = job;
        heldCount_ = jobs_[job] - k;
        keyboardHold_ = false;
        return;
    }
    for (const Rect& r : {kReturnButton, kChangeButton, kBuyButton}) {
        if (r.contains(x, y)) {
            held_ = r;
        }
    }
}

ColonyScreen::Action ColonyScreen::release(int x, int y) {
    pointerMoved(x, y);
    if (heldCount_ > 0 && !keyboardHold_) {
        bool inside = false;
        const std::size_t row = rowAt(y, inside);
        if (!inside || x < kPeopleX - kPersonStep || x >= kRowRight) {
            heldCount_ = 0;
            return Action::None;
        }
        return drop(row);
    }
    const Rect held = held_;
    held_ = Rect{};
    if (!held.contains(x, y)) {
        return Action::None;
    }
    return held.x == kReturnButton.x ? Action::Return : held.x == kChangeButton.x ? Action::Change : Action::Buy;
}

Palette ColonyScreen::palette() const {
    Palette p = base_;
    sky_.applyPalette(p);
    landscape_.applyPalette(p);
    frame_.applyPalette(p);
    return p;
}

void ColonyScreen::drawShaded(const Image& picture, Canvas& canvas) const {
    const Canvas under = canvas;
    picture.draw(0, canvas);
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const std::uint8_t v = canvas.at(x, y);
            if ((v == kShadow || v == kShadow2) && v != under.at(x, y)) {
                canvas.set(x, y, shade_[under.at(x, y)]);
            }
        }
    }
}

void ColonyScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = this->palette();
    for (int y = 0; y < canvas.height() && y < scene_.height(); ++y) {
        for (int x = 0; x < canvas.width() && x < scene_.width(); ++x) {
            canvas.set(x, y, scene_.at(x, y));
        }
    }
    frame_.draw(0, canvas);

    const Font& listFont = fonts_.font(kListFont);
    for (std::size_t k = 0; k < kOrbits; ++k) {
        const Orbit& o = orbits_[k];
        const int y = kListY + kOrbitStep * static_cast<int>(k);
        arrow_.draw(o.shown ? arrowFrame_ : 0, canvas, kArrowX, y);
        if (!o.planet) {
            continue;
        }
        const int centre = kIconCentreY + kOrbitStep * static_cast<int>(k);
        o.icon.draw(0, canvas, kIconCentreX - o.icon.width() / 2, centre - o.icon.height() / 2);
        const TextStyle style = listStyle(o.shown);
        listFont.draw(canvas, kListX, y, o.lines[0], style);
        listFont.draw(canvas, kListX, y + kSecondLine, o.lines[1], style);
    }

    for (std::size_t r = 0; r < kRows; ++r) {
        const int step = iconStep(rows_[r].size());
        for (std::size_t i = 0; i < rows_[r].size(); ++i) {
            if (rows_[r][i] != kGap) {
                icons_[static_cast<std::size_t>(rows_[r][i])].draw(0, canvas, kRowX + step * static_cast<int>(i), kRowY[r]);
            }
        }
    }
    icons_[static_cast<std::size_t>(kGovernmentIcon + government_)].draw(0, canvas, kGovernmentX, kGovernmentY);
    for (std::size_t j = 0; j < people_.size(); ++j) {
        // Closer together when they don't fit (assumed).
        const int n = jobs_[j] - (heldCount_ > 0 && heldJob_ == j ? heldCount_ : 0);
        const int step = personStep(jobs_[j]);
        for (int k = 0; k < n; ++k) {
            people_[j].draw(0, canvas, kPeopleX + step * k, kPeopleY[j]);
        }
    }
    if (heldCount_ > 0) {
        int x = pointerX_ + kHeldDX, y = pointerY_ + kHeldDY;
        std::size_t job = heldJob_;
        if (keyboardHold_) {
            // Hegemon: after the chosen row's people.
            job = cursor_;
            const int n = jobs_[cursor_] - (heldJob_ == cursor_ ? heldCount_ : 0);
            x = kPeopleX + personStep(jobs_[cursor_]) * n;
            y = kPeopleY[cursor_];
        } else {
            bool inside = false;
            const std::size_t row = rowAt(pointerY_, inside);
            if (inside) {
                job = row;
            }
        }
        for (int k = 0; k < heldCount_; ++k) {
            people_[job].draw(0, canvas, x + kHeldStep * k, y);
        }
    }

    TextStyle style;
    style.shades[1] = kShade1;
    style.shades[2] = kShade2;
    style.shades[3] = kShade3;
    style.spacing = kSpacing;
    const Font& titleFont = fonts_.font(kTitleFont);
    titleFont.draw(canvas, kTitleCentre - titleFont.textWidth(title_, kSpacing) / 2, kTitleY, title_, style);
    if (picture_) {
        picture_->draw(canvas, ProductPicture::Box::Colony, palette);
    }
    {
        TextStyle box;
        box.shades[1] = kShade1;
        box.shades[2] = kShade2;
        box.shades[3] = kShade3;
        box.shadow = std::uint8_t{0};
        box.spacing = kListSpacing;
        const Font& nameFont = fonts_.font(kProductFont);
        nameFont.draw(canvas, kProductCentre - nameFont.textWidth(product_, kListSpacing) / 2, kProductY, product_, box);
        const Font& boxFont = fonts_.font(kBoxFont);
        boxFont.draw(canvas, kTurnsX, kBoxTextY, turns_, box);
        boxFont.draw(canvas, kProgressRight - boxFont.textWidth(progress_, kListSpacing), kBoxTextY, progress_, box);
    }
    const Font& popFont = fonts_.font(kPopulationFont);
    popFont.draw(canvas, kPopulationRight - popFont.textWidth(population_, kSpacing), kPopulationY, population_, style);
}

} // namespace hegemon
