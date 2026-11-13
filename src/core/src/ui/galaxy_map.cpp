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
// Home star names: font 2 in the banner's colour under the star.
constexpr std::size_t kNameFont = 2;
constexpr int kNameSpacing = 1;
constexpr int kNameBelow = 11; // the name's top, below the star's middle
constexpr std::uint8_t kNameOutline = 2;
// The banner colours names are written in, red to orange.
constexpr std::array<std::uint8_t, 8> kBannerText = {79, 79, 79, 79, 79, 79, 79, 79};
constexpr std::array<std::uint8_t, 8> kBannerTextDark = {74, 74, 74, 74, 74, 74, 74, 74};

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
    const auto& sky = data.archive(kSkyArchive);
    for (std::size_t i = 0; i < kNebulaPictures; ++i) {
        nebulae_.push_back(
            Image::parse(sky.entry(kFirstNebula + i * kNebulaScales + static_cast<std::size_t>(scale_))));
    }
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
        const int x = screenX(s.x), y = screenY(s.y);
        const Image& picture =
            s.colour == StarColour::BlackHole
                ? blackHole_
                : starSizes_[static_cast<std::size_t>(s.colour) * 3 + static_cast<std::size_t>(std::clamp(s.size, 0, 2))];
        picture.draw(0, canvas, x - (picture.width() - 1) / 2, y - (picture.height() - 1) / 2);
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
        style.shadow = kNameOutline;
        style.shadowAt = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
        // Centred on the letters, without the spacing after the last one.
        const int width = font.textWidth(s.name, kNameSpacing) - kNameSpacing;
        font.draw(canvas, screenX(s.x) - width / 2, screenY(s.y) + kNameBelow, s.name, style);
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
}

} // namespace hegemon
