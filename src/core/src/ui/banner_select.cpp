// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/banner_select.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/races.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/banner.md).
constexpr const char* kBackgroundArchive = "RACEOPT.LBX";
constexpr std::size_t kBackgroundEntry = 0;
constexpr const char* kArchive = "RACESEL.LBX";
constexpr std::size_t kPanelEntry = 32;
constexpr int kPanelX = 55, kPanelY = 67;
constexpr std::size_t kFirstBanner = 34; // eight for each race
// The banners differ in size by a pixel or two; each is drawn from here.
constexpr std::array<int, 4> kColumnX = {96, 220, 344, 467};
constexpr std::array<int, 2> kRowY = {145, 281};

constexpr const char* kStringsArchive = "HESTRNGS.LBX";
constexpr std::size_t kGeneratingString = 242; // "Generating Universe ..."
constexpr std::size_t kTextFont = 4;
constexpr int kTextSpacing = 2;
constexpr int kTextY = 215;
constexpr std::array<std::uint8_t, 3> kTextShades = {119, 124, 123};

} // namespace

BannerSelectScreen::BannerSelectScreen(GameData& data, std::size_t race)
    : background_(Image::parse(data.archive(kBackgroundArchive).entry(kBackgroundEntry))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))) {
    if (race >= Races::kStockRaces) {
        throw std::out_of_range("no banners for this race");
    }
    const auto& archive = data.archive(kArchive);
    for (std::size_t i = 0; i < kBanners; ++i) {
        banners_.push_back(Image::parse(archive.entry(kFirstBanner + race * kBanners + i)));
    }
}

Rect BannerSelectScreen::bannerRect(std::size_t banner) const {
    const auto& img = banners_.at(banner);
    return {kColumnX[banner % kColumnX.size()], kRowY[banner / kColumnX.size()], img.width(), img.height()};
}

std::optional<std::size_t> BannerSelectScreen::bannerAt(int x, int y) const {
    for (std::size_t i = 0; i < kBanners; ++i) {
        if (bannerRect(i).contains(x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

void BannerSelectScreen::press(int, int) { held_ = true; }

std::optional<BannerSelectScreen::Colour> BannerSelectScreen::release(int x, int y) {
    if (!std::exchange(held_, false)) {
        return std::nullopt;
    }
    if (const auto at = bannerAt(x, y)) {
        return static_cast<Colour>(*at);
    }
    return std::nullopt;
}

std::optional<BannerSelectScreen::Colour> BannerSelectScreen::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

void BannerSelectScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    background_.draw(0, canvas);
    panel_.applyPalette(palette);
    panel_.draw(0, canvas, kPanelX, kPanelY);
    for (std::size_t i = 0; i < kBanners; ++i) {
        const Rect r = bannerRect(i);
        banners_[i].draw(0, canvas, r.x, r.y);
    }
}

GeneratingScreen::GeneratingScreen(GameData& data)
    : background_(Image::parse(data.archive(kBackgroundArchive).entry(kBackgroundEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto strings = parseStringList(data.archive(kStringsArchive).entry(0));
    if (strings.size() <= kGeneratingString) {
        throw FormatError("too few messages in HESTRNGS.LBX");
    }
    text_ = strings[kGeneratingString];
}

void GeneratingScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    background_.draw(0, canvas);
    TextStyle style;
    for (std::size_t i = 0; i < kTextShades.size(); ++i) {
        style.shades[i + 1] = kTextShades[i];
    }
    style.spacing = kTextSpacing;
    const Font& font = fonts_.font(kTextFont);
    font.draw(canvas, (canvas.width() - font.textWidth(text_, kTextSpacing)) / 2, kTextY, text_, style);
}

} // namespace hegemon
