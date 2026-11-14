// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/landing_screen.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colonizing.md). The colony screen's
// sky and landscape and its colours (FONTS.LBX entry 2), and the ship
// coming down: COLONY.LBX entry 6, 73 frames, one every 1/20 second, each
// drawn whole over the landscape; its colours 240 and 241 are shadow, as
// on the colony screen.
constexpr std::size_t kColoursEntry = 2;
constexpr const char* kSkyArchive = "COLONY2.LBX";
constexpr std::size_t kSkyEntry = 49;
constexpr const char* kLandscapeArchive = "PLANETS.LBX";
constexpr const char* kLandingArchive = "COLONY.LBX";
constexpr std::size_t kLandingEntry = 6;
constexpr std::uint64_t kFrameMs = 50;
constexpr std::uint8_t kShadow = 240, kShadow2 = 241;
// The texts in font 4, two pixels between letters, shades 1 to 3 in
// colours 245, 253 and 252: the title centred on x 319 at y 10 from the
// start, the lines from (20, 40) 20 pixels apart once the ship is down.
constexpr std::size_t kFont = 4;
constexpr int kSpacing = 2, kTitleCentre = 319, kTitleY = 10, kLinesX = 20, kLinesY = 40, kLineStep = 20;
constexpr std::uint8_t kShade1 = 245, kShade2 = 253, kShade3 = 252;
// ESTRINGS.LBX.
constexpr std::size_t kTitle = 229, kColonyLine = 40, kPopulationLine = 31, kMineralLine = 374, kRaceNames = 645;
constexpr std::array<std::size_t, kClimates> kClimateWords = {539, 719, 720, 721, 722, 399, 501, 184, 723, 303};
constexpr std::array<std::size_t, 5> kSizeWords = {683, 480, 371, 360, 323};
constexpr std::array<std::size_t, 5> kMineralWords = {684, 423, 685, 450, 686};
constexpr std::array<std::size_t, 3> kGravityWords = {687, 688, 689};
constexpr std::size_t kLastWord = 723;
// HESTRNGS.LBX: the word for a star's first planet.
constexpr std::size_t kPrime = 160;

std::string trimmed(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

} // namespace

LandingScreen::LandingScreen(GameData& data, const Game& game, std::size_t colony)
    : sky_(Image::parse(data.archive(kSkyArchive).entry(kSkyEntry))),
      landscape_([&] {
          const Colony& c = game.colonies.at(colony);
          const Planet& p = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
          const auto climate = std::min<std::size_t>(static_cast<std::size_t>(p.climate), kClimates - 1);
          const auto image = static_cast<std::size_t>(std::clamp(p.image, 0, kPlanetImages - 1));
          return Image::parse(data.archive(kLandscapeArchive).entry(climate * kPlanetImages + image));
      }()),
      landing_(Image::parse(data.archive(kLandingArchive).entry(kLandingEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto colours = data.archive("FONTS.LBX").entry(kColoursEntry);
    if (colours.size() < 1024) {
        throw FormatError("FONTS.LBX: the colony screen's colours are missing");
    }
    palette_.set(0, colours.subspan(0, 1024));
    sky_.applyPalette(palette_);
    landscape_.applyPalette(palette_);
    landing_.applyPalette(palette_);
    // A shadow darkens to the colour nearest half as bright (as on the
    // colony screen).
    const auto six = [&](std::size_t i, int channel) {
        const Color& col = palette_.colors[i];
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

    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (messages.size() <= std::max(kLastWord, kRaceNames + 12) || strings.size() <= kPrime) {
        throw FormatError("too few messages in ESTRINGS.LBX or HESTRNGS.LBX");
    }
    const Colony& c = game.colonies.at(colony);
    const Planet& p = game.galaxy.planets.at(static_cast<std::size_t>(c.planet));
    const Player& owner = game.players.at(static_cast<std::size_t>(c.player));
    const std::string name = planetName(game.galaxy, c.planet, strings[kPrime]);
    title_ = trimmed(formatMessage(messages[kTitle], {name}));
    const std::string race = messages[kRaceNames + static_cast<std::size_t>(std::clamp(owner.race, 0, 12))];
    lines_.push_back(trimmed(formatMessage(messages[kColonyLine], {name, race})));
    const auto size = std::min<std::size_t>(static_cast<std::size_t>(p.size), kSizeWords.size() - 1);
    const auto climate = std::min<std::size_t>(static_cast<std::size_t>(p.climate), kClimates - 1);
    lines_.push_back(trimmed(messages[kSizeWords[size]]) + " " + trimmed(messages[kClimateWords[climate]]));
    const long long most = Economy(data.parameters()).maxPopulation(owner, p);
    lines_.push_back(trimmed(formatMessage(messages[kPopulationLine], {most})));
    const auto minerals = std::min<std::size_t>(static_cast<std::size_t>(p.minerals), kMineralWords.size() - 1);
    lines_.push_back(trimmed(formatMessage(messages[kMineralLine], {trimmed(messages[kMineralWords[minerals]])})));
    const auto gravity = std::min<std::size_t>(static_cast<std::size_t>(p.gravity), kGravityWords.size() - 1);
    lines_.push_back(trimmed(messages[kGravityWords[gravity]]));
}

std::size_t LandingScreen::frame() const {
    return std::min<std::size_t>(static_cast<std::size_t>(time_ / kFrameMs), landing_.frameCount() - 1);
}

bool LandingScreen::landed() const { return frame() + 1 >= landing_.frameCount(); }

void LandingScreen::skip() { time_ = kFrameMs * landing_.frameCount(); }

void LandingScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = palette_;
    Canvas scene(canvas.width(), canvas.height(), 0);
    sky_.draw(0, scene);
    landscape_.draw(0, scene);
    const Canvas under = scene;
    landing_.draw(frame(), scene);
    for (int y = 0; y < scene.height(); ++y) {
        for (int x = 0; x < scene.width(); ++x) {
            const std::uint8_t v = scene.at(x, y);
            if ((v == kShadow || v == kShadow2) && v != under.at(x, y)) {
                scene.set(x, y, shade_[under.at(x, y)]);
            }
            canvas.set(x, y, scene.at(x, y));
        }
    }
    TextStyle style;
    style.spacing = kSpacing;
    style.shades[1] = kShade1;
    style.shades[2] = kShade2;
    style.shades[3] = kShade3;
    const Font& font = fonts_.font(kFont);
    font.draw(canvas, kTitleCentre - font.textWidth(title_, kSpacing) / 2, kTitleY, title_, style);
    if (landed()) {
        for (std::size_t i = 0; i < lines_.size(); ++i) {
            font.draw(canvas, kLinesX, kLinesY + kLineStep * static_cast<int>(i), lines_[i], style);
        }
    }
}

} // namespace hegemon
