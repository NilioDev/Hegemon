// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/galaxy_map.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/galaxy-map.md).
constexpr std::size_t kPaletteEntry = 1; // of FONTS.LBX: the game's colours
constexpr const char* kArchive = "BUFFER0.LBX";
constexpr std::size_t kFrameEntry = 0;
// Six pictures for each star colour (blue to brown), from biggest to
// smallest; a galaxy map shows three of them, the bigger the smaller the
// galaxy.
constexpr std::size_t kFirstStar = 148;
constexpr std::size_t kStarPictures = 6;
// Black holes: one picture for each scale from here (the medium one
// measured, the others assumed), 16 frames of an animation.
constexpr std::size_t kFirstBlackHole = 184;
constexpr const char* kSkyArchive = "STARBG.LBX";
constexpr std::size_t kSkyEntry = 3;
constexpr std::size_t kFirstNebula = 6; // twelve, each at four scales
constexpr std::size_t kNebulaPictures = 12;
constexpr std::size_t kNebulaScales = 4;
// The map's corner on the screen, and how wide and high the whole galaxy is
// drawn there.
constexpr int kMapX = 21, kMapY = 21;
constexpr int kMapW = 506, kMapH = 400;
constexpr Rect kMapClip = {21, 21, 506, 401};
// The dark outline of the map's texts: the text drawn around itself in this
// order, each shade in its own dark colour (measured).
constexpr std::uint8_t kOutline1 = 1, kOutline2 = 2;
const std::vector<ShadowOffset> kOutlineOrder = {{-1, 0}, {1, 1}, {-1, -1}, {0, -1}, {1, 0}, {0, 1}, {1, -1}, {-1, 1}};
// Home star names: font 2 in the banner's colour under the star.
constexpr std::size_t kNameFont = 2;
constexpr int kNameSpacing = 1;
// The name's top, below the star's middle, by the star's size (measured on
// the medium map; assumed the same on the others).
constexpr std::array<int, 3> kNameBelow = {14, 11, 9};
// The banner colours names are written in, red to orange.
constexpr std::array<std::uint8_t, 8> kBannerText = {79, 79, 79, 79, 79, 79, 79, 79};
constexpr std::array<std::uint8_t, 8> kBannerTextDark = {74, 74, 74, 74, 74, 74, 74, 74};

// The fleet marker at a star: entry 208 - scale, a frame per banner, its
// top left corner this far from the star picture's top right one (the
// medium map measured with all three star sizes, the small and large maps
// with one, the huge one assumed like the large one).
constexpr std::size_t kFleetLast = 208;
constexpr std::array<std::array<int, 2>, 4> kFleetOffset = {{{-6, 0}, {-5, -1}, {-6, -2}, {-6, -2}}};

// The side panel's texts: font 2 in two greys, centred on x 579 (the
// stardate on 580).
constexpr int kPanelSpacing = 2;
constexpr std::uint8_t kPanelShade1 = 14, kPanelShade2 = 23;
constexpr int kPanelCentre = 579, kStardateCentre = 580;
constexpr int kStardateY = 29, kMoneyY = 96, kIncomeY = 108, kCommandY = 181, kFoodY = 255, kFreightY = 330,
              kResearchY = 403;
// The research project's estimate, above its research points.
constexpr int kTurnsCentre = 584, kTurnsY = 391, kWaveDrop = 3;
// Command points and freighters are two texts: the first ends at the
// first x (with its spacing), the second starts at the other.
constexpr int kCommandSplit[2] = {578, 582};
constexpr int kFreightSplit[2] = {576, 578};
constexpr std::size_t kNoneText = 392; // HESTRNGS.LBX: "none"

std::string signedNumber(int n) { return (n >= 0 ? "+" : "") + std::to_string(n); }

// The Enter Home Star Name box: its corner, its title (HESTRNGS.LBX) and
// its texts' colours.
constexpr int kNameBoxX = 131, kNameBoxY = 146;
constexpr std::size_t kNameBoxTitle = 257;
constexpr NameBox::Shades kNameBoxShades = {{12, 24, 19}, {12, 24}};

} // namespace

NameBox homeStarNameBox(GameData& data, std::string name) {
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings.size() <= kNameBoxTitle) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    return NameBox(data, strings[kNameBoxTitle], std::move(name), kNameBoxX, kNameBoxY, kNameBoxShades);
}

GalaxyMap::GalaxyMap(GameData& data, const Galaxy& galaxy, std::vector<std::size_t> banners)
    : galaxy_(galaxy), banners_(std::move(banners)), frame_(Image::parse(data.archive(kArchive).entry(kFrameEntry))),
      sky_(Image::parse(data.archive(kSkyArchive).entry(kSkyEntry))),
      blackHole_(Image::parse(data.archive(kArchive).entry(
          kFirstBlackHole + static_cast<std::size_t>(std::clamp(galaxy.mapSize, 0, static_cast<int>(kNebulaScales) - 1))))),
      fleet_(Image::parse(data.archive(kArchive).entry(
          kFleetLast - static_cast<std::size_t>(std::clamp(galaxy.mapSize, 0, static_cast<int>(kNebulaScales) - 1))))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))),
      scale_(std::clamp(galaxy.mapSize, 0, static_cast<int>(kNebulaScales) - 1)) {
    if (galaxy.width <= 0 || galaxy.height <= 0) {
        throw std::invalid_argument("a galaxy map needs a galaxy with a size");
    }
    const auto palette = data.archive("FONTS.LBX").entry(kPaletteEntry);
    if (palette.size() < 1024) {
        throw FormatError("FONTS.LBX: the game's colours are missing");
    }
    palette_.set(0, palette.subspan(0, 1024));
    frame_.applyPalette(palette_);
    const auto& stars = data.archive(kArchive);
    for (std::size_t colour = 0; colour + 1 < kStarColours; ++colour) {
        for (std::size_t size = 0; size < 3; ++size) {
            starSizes_.push_back(
                Image::parse(stars.entry(kFirstStar + colour * kStarPictures + static_cast<std::size_t>(scale_) + size)));
        }
    }
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (strings.size() <= kNoneText) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    none_ = strings[kNoneText];
    const auto& sky = data.archive(kSkyArchive);
    for (std::size_t i = 0; i < kNebulaPictures; ++i) {
        nebulae_.push_back(
            Image::parse(sky.entry(kFirstNebula + i * kNebulaScales + static_cast<std::size_t>(scale_))));
    }
}

const Image& GalaxyMap::starPicture(const Star& s) const {
    if (s.colour == StarColour::BlackHole) {
        return blackHole_;
    }
    const auto colour = std::min<std::size_t>(static_cast<std::size_t>(s.colour), kStarColours - 2);
    return starSizes_[colour * 3 + static_cast<std::size_t>(std::clamp(s.size, 0, 2))];
}

void GalaxyMap::setPanel(int stardate, const EmpireSummary& summary) {
    stardate_ = stardate;
    summary_ = summary;
    panel_ = true;
}

int GalaxyMap::screenX(int x) const { return kMapX + x * kMapW / galaxy_.width; }

int GalaxyMap::screenY(int y) const { return kMapY + y * kMapH / galaxy_.height; }

void GalaxyMap::draw(Canvas& canvas, Palette& palette) const {
    palette = palette_;
    sky_.draw(0, canvas);
    for (const Nebula& n : galaxy_.nebulae) {
        if (n.picture >= 0 && static_cast<std::size_t>(n.picture) < nebulae_.size()) {
            nebulae_[static_cast<std::size_t>(n.picture)].draw(0, canvas, screenX(n.x), screenY(n.y));
        }
    }
    for (const Star& s : galaxy_.stars) {
        const Image& picture = starPicture(s);
        picture.draw(0, canvas, screenX(s.x) - (picture.width() - 1) / 2, screenY(s.y) - (picture.height() - 1) / 2);
    }
    // The human's fleets, in their banner's colour.
    if (!banners_.empty() && fleet_.frameCount() > 0) {
        const auto frame = std::min(banners_[0], fleet_.frameCount() - 1);
        const auto& offset = kFleetOffset[static_cast<std::size_t>(scale_)];
        for (const int star : fleets_) {
            if (star >= 0 && static_cast<std::size_t>(star) < galaxy_.stars.size()) {
                const Star& s = galaxy_.stars[static_cast<std::size_t>(star)];
                const Image& picture = starPicture(s);
                fleet_.draw(frame, canvas, screenX(s.x) + (picture.width() - 1) / 2 + offset[0],
                            screenY(s.y) - (picture.height() - 1) / 2 + offset[1]);
            }
        }
    }
    // At first the player knows only the name of their own home star.
    const Font& font = fonts_.font(kNameFont);
    for (std::size_t i = 0; i < 1 && i < galaxy_.homes.size() && i < banners_.size(); ++i) {
        const Star& s = galaxy_.stars.at(static_cast<std::size_t>(galaxy_.homes[i].star));
        const std::size_t banner = std::min<std::size_t>(banners_[i], kBannerText.size() - 1);
        TextStyle style;
        style.shades[1] = kBannerTextDark[banner];
        style.shades[2] = kBannerText[banner];
        style.spacing = kNameSpacing;
        style.shadowShades[1] = kOutline1;
        style.shadowShades[2] = kOutline2;
        style.shadowAt = kOutlineOrder;
        // Centred on the letters, without the spacing after the last one.
        const int width = font.textWidth(s.name, kNameSpacing) - kNameSpacing;
        font.draw(canvas, screenX(s.x) - width / 2,
                  screenY(s.y) + kNameBelow[static_cast<std::size_t>(std::clamp(s.size, 0, 2))], s.name, style);
    }
    // The map leaves the frame opening's left, top and right edges black.
    for (int y = kView.y; y < kView.y + kView.h; ++y) {
        for (int x = kView.x; x < kView.x + kView.w; ++x) {
            if (!kMapClip.contains(x, y) && y < canvas.height() && x < canvas.width()) {
                canvas.set(x, y, 0);
            }
        }
    }
    frame_.draw(0, canvas);
    if (panel_) {
        drawPanel(canvas);
    }
}

void GalaxyMap::drawPanel(Canvas& canvas) const {
    const Font& font = fonts_.font(kNameFont);
    TextStyle style;
    style.shades[1] = kPanelShade1;
    style.shades[2] = kPanelShade2;
    style.spacing = kPanelSpacing;
    style.shadowShades[1] = kOutline1;
    style.shadowShades[2] = kOutline2;
    style.shadowAt = kOutlineOrder;
    const auto centred = [&](const std::string& text, int centre, int y) {
        font.draw(canvas, centre - font.textWidth(text, kPanelSpacing) / 2, y, text, style);
    };
    const auto split = [&](const std::string& left, const std::string& right, const int (&at)[2], int y) {
        font.draw(canvas, at[0] - font.textWidth(left, kPanelSpacing), y, left, style);
        font.draw(canvas, at[1], y, right, style);
    };
    centred(std::to_string(stardate_ / 10) + "." + std::to_string(stardate_ % 10), kStardateCentre, kStardateY);
    centred(std::to_string(summary_.money) + " BC", kPanelCentre, kMoneyY);
    centred(signedNumber(summary_.income) + " BC", kPanelCentre, kIncomeY);
    // Measured: the command points' total has no outline.
    TextStyle plain = style;
    plain.shadowShades = {};
    font.draw(canvas, kCommandSplit[0] - font.textWidth(signedNumber(summary_.commandPoints - summary_.commandUsed), kPanelSpacing),
              kCommandY, signedNumber(summary_.commandPoints - summary_.commandUsed), style);
    font.draw(canvas, kCommandSplit[1], kCommandY, "(" + std::to_string(summary_.commandPoints) + ")", plain);
    centred(signedNumber(summary_.food), kPanelCentre, kFoodY);
    split(signedNumber(summary_.freighters - summary_.freightersUsed), "(" + std::to_string(summary_.freighters) + ")",
          kFreightSplit, kFreightY);
    if (summary_.researchField == 0) {
        centred(none_, kPanelCentre, kResearchY);
        return;
    }
    // Measured: the estimate starts with the font's wavy line (its "@"),
    // drawn three pixels lower than the text, and is centred a little
    // further right, its odd pixel to the left.
    const std::string turns = std::to_string(summary_.researchTurns) + " turns";
    const std::string wave = "@";
    const int whole = font.textWidth(wave + turns, kPanelSpacing);
    const int x = kTurnsCentre - (whole + 1) / 2;
    font.draw(canvas, x, kTurnsY + kWaveDrop, wave, style);
    font.draw(canvas, x + whole - font.textWidth(turns, kPanelSpacing), kTurnsY, turns, style);
    centred(std::to_string(summary_.research) + " RP", kPanelCentre, kResearchY);
}

} // namespace hegemon
