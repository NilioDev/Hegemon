// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/colony_screen.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

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
// ESTRINGS.LBX messages.
constexpr std::size_t kTitleMessage = 97;      // "%sColony of %s"
constexpr std::size_t kPopulationMessage = 424; // "Pop %d,%03d k (%+dk)"
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
    population_ = formatMessage(messages[kPopulationMessage],
                                {static_cast<long long>(c.population()), static_cast<long long>(c.growth),
                                 static_cast<long long>(economy.growth(player, planet, c.population()))});
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

void ColonyScreen::press(int x, int y) {
    held_ = kReturnButton.contains(x, y);
}

bool ColonyScreen::release(int x, int y) {
    const bool done = held_ && kReturnButton.contains(x, y);
    held_ = false;
    return done;
}

void ColonyScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = base_;
    sky_.applyPalette(palette);
    landscape_.applyPalette(palette);
    frame_.applyPalette(palette);
    // The sky shows where the landscape leaves it.
    sky_.draw(0, canvas);
    landscape_.draw(0, canvas);
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
        const int n = jobs_[j];
        const int step = n < 2 ? kPersonStep : std::min(kPersonStep, kPeopleWidth / (n - 1));
        for (int k = 0; k < n; ++k) {
            people_[j].draw(0, canvas, kPeopleX + step * k, kPeopleY[j]);
        }
    }

    TextStyle style;
    style.shades[1] = kShade1;
    style.shades[2] = kShade2;
    style.shades[3] = kShade3;
    style.spacing = kSpacing;
    const Font& titleFont = fonts_.font(kTitleFont);
    titleFont.draw(canvas, kTitleCentre - titleFont.textWidth(title_, kSpacing) / 2, kTitleY, title_, style);
    const Font& popFont = fonts_.font(kPopulationFont);
    popFont.draw(canvas, kPopulationRight - popFont.textWidth(population_, kSpacing), kPopulationY, population_, style);
}

} // namespace hegemon
