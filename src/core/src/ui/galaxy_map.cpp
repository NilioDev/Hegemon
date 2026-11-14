// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/galaxy_map.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>
#include <cstdlib>
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
// A fleet sent off but still at its star: its top left corner this far
// left of and above the star picture's top left one (measured on the
// medium map with one star).
constexpr int kLeavingDx = -4, kLeavingDy = -2;
// The course of a fleet on its way: from the marker's middle to the star's,
// dashes of six in three greens and gaps of two in a darker one, moving
// along (measured).
constexpr std::array<std::uint8_t, 8> kDashes = {110, 110, 111, 114, 116, 116, 114, 111};
// "eta N" (HESTRNGS.LBX) under a fleet on its way: font 1 in the banner's
// colours, its left 7 pixels left of the marker's, its top under it
// (measured with one fleet).
constexpr std::size_t kEtaText = 307;
constexpr std::size_t kEtaFont = 1;
constexpr int kEtaDx = -7;
// The names of explored stars no one of the human's owns: grey.
constexpr std::uint8_t kExploredName = 12, kExploredNameDark = 1;

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

// A message with its "%d" filled in.
std::string formatNumber(const std::string& message, int n) {
    std::string text = message;
    const auto at = text.find("%d");
    if (at != std::string::npos) {
        text.replace(at, 2, std::to_string(n));
    }
    return text;
}

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
    if (strings.size() <= kEtaText) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    eta_ = strings[kEtaText];
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
    for (const Marker& f : fleets_) {
        drawCourse(canvas, f);
    }
    for (const Star& s : galaxy_.stars) {
        const Image& picture = starPicture(s);
        picture.draw(0, canvas, screenX(s.x) - (picture.width() - 1) / 2, screenY(s.y) - (picture.height() - 1) / 2);
    }
    // The human's fleets, in their banner's colour, over their courses.
    if (!banners_.empty() && fleet_.frameCount() > 0) {
        const auto frame = std::min(banners_[0], fleet_.frameCount() - 1);
        for (const Marker& f : fleets_) {
            const Rect r = markerRect(f);
            fleet_.draw(frame, canvas, r.x, r.y);
        }
        const Font& small = fonts_.font(kEtaFont);
        const std::size_t banner = std::min<std::size_t>(banners_[0], kBannerText.size() - 1);
        TextStyle style;
        style.shades[1] = kBannerTextDark[banner];
        style.shades[2] = kBannerText[banner];
        style.spacing = 1;
        for (const Marker& f : fleets_) {
            if (f.star < 0 && f.destination >= 0) {
                const Rect r = markerRect(f);
                small.draw(canvas, r.x + kEtaDx, r.y + r.h, formatNumber(eta_, f.eta), style);
            }
        }
    }
    // The names of the stars the human knows: their home's in the banner's
    // colour, the others they explored in grey.
    if (!galaxy_.homes.empty() && !banners_.empty()) {
        const int home = galaxy_.homes[0].star;
        for (std::size_t i = 0; i < galaxy_.stars.size() && i < explored_.size(); ++i) {
            if (explored_[i] && static_cast<int>(i) != home) {
                drawName(canvas, galaxy_.stars[i], kExploredNameDark, kExploredName);
            }
        }
        const std::size_t banner = std::min<std::size_t>(banners_[0], kBannerText.size() - 1);
        if (home >= 0 && static_cast<std::size_t>(home) < galaxy_.stars.size()) {
            drawName(canvas, galaxy_.stars[static_cast<std::size_t>(home)], kBannerTextDark[banner], kBannerText[banner]);
        }
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

Rect GalaxyMap::markerRect(const Marker& fleet) const {
    const int w = fleet_.width(), h = fleet_.height();
    if (fleet.star >= 0 && static_cast<std::size_t>(fleet.star) < galaxy_.stars.size()) {
        const Star& s = galaxy_.stars[static_cast<std::size_t>(fleet.star)];
        const Image& picture = starPicture(s);
        const auto& offset = kFleetOffset[static_cast<std::size_t>(scale_)];
        const int top = screenY(s.y) - (picture.height() - 1) / 2;
        if (fleet.destination >= 0) {
            return {screenX(s.x) - (picture.width() - 1) / 2 + kLeavingDx, top + kLeavingDy, w, h};
        }
        return {screenX(s.x) + (picture.width() - 1) / 2 + offset[0], top + offset[1], w, h};
    }
    // On its way: centred on its place.
    return {screenX(fleet.x) - w / 2, screenY(fleet.y) - h / 2, w, h};
}

void GalaxyMap::drawCourse(Canvas& canvas, const Marker& fleet) const {
    if (fleet.destination < 0 || static_cast<std::size_t>(fleet.destination) >= galaxy_.stars.size()) {
        return;
    }
    const Rect r = markerRect(fleet);
    const Star& target = galaxy_.stars[static_cast<std::size_t>(fleet.destination)];
    const int x0 = r.x + r.w / 2, y0 = r.y + r.h / 2;
    const int x1 = screenX(target.x), y1 = screenY(target.y);
    const int dx = x1 - x0, dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    for (int i = 0; i <= steps; ++i) {
        // Each step along the longer side, the other side rounded to the
        // nearest pixel.
        const int x = steps == 0 ? x0 : x0 + (2 * dx * i + (dx < 0 ? -steps : steps)) / (2 * steps);
        const int y = steps == 0 ? y0 : y0 + (2 * dy * i + (dy < 0 ? -steps : steps)) / (2 * steps);
        const int n = static_cast<int>(kDashes.size());
        const std::uint8_t colour = kDashes[static_cast<std::size_t>(((i - dashStep_) % n + n) % n)];
        if (kMapClip.contains(x, y)) {
            canvas.set(x, y, colour);
        }
    }
}

void GalaxyMap::drawName(Canvas& canvas, const Star& s, std::uint8_t dark, std::uint8_t light) const {
    const Font& font = fonts_.font(kNameFont);
    TextStyle style;
    style.shades[1] = dark;
    style.shades[2] = light;
    style.spacing = kNameSpacing;
    style.shadowShades[1] = kOutline1;
    style.shadowShades[2] = kOutline2;
    style.shadowAt = kOutlineOrder;
    // Centred on the letters, without the spacing after the last one.
    const int width = font.textWidth(s.name, kNameSpacing) - kNameSpacing;
    font.draw(canvas, screenX(s.x) - width / 2, screenY(s.y) + kNameBelow[static_cast<std::size_t>(std::clamp(s.size, 0, 2))],
              s.name, style);
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
