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
