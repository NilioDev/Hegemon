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
    if (messages.size() <= kPopulationMessage) {
        throw FormatError("too few messages in ESTRINGS.LBX");
    }
    title_ = formatMessage(messages[kTitleMessage],
                           {std::string(), planetName(game.galaxy, c.planet, strings[kPrimeText])});
    population_ = formatMessage(messages[kPopulationMessage],
                                {static_cast<long long>(c.population()), static_cast<long long>(c.growth),
                                 static_cast<long long>(economy.growth(player, planet, c.population()))});
}

int ColonyScreen::iconStep(std::size_t places) {
    if (places < 2) {
        return kMaxStep;
    }
    return std::min(kMaxStep, kRowWidth / static_cast<int>(places - 1));
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
