// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/race_select.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"

namespace hegemon {

namespace {

// Measured on the original (docs/rules/race-select.md).
constexpr const char* kArchive = "RACESEL.LBX";
constexpr const char* kBackgroundArchive = "RACEOPT.LBX";
constexpr std::size_t kBackgroundEntry = 0;
constexpr std::size_t kPanelEntry = 0;
constexpr int kPanelX = 48, kPanelY = 58;
constexpr std::size_t kFirstButton = 1;
constexpr std::size_t kFirstPortrait = 15;
constexpr int kPortraitX = 54, kPortraitY = 63;
constexpr std::size_t kTitleEntry = 33;
constexpr int kTitleX = 366, kTitleY = 52;
// Two columns of seven buttons.
constexpr std::array<int, 2> kColumnX = {351, 477};
constexpr int kFirstRowY = 90;
constexpr int kRowStep = 48;
constexpr std::size_t kRows = 7;

// The text under the portrait: font 2, two pixels between letters, shades
// in colours 120 and 124 over a black shadow to the right, below and
// right below, lines 13 pixels apart.
constexpr std::size_t kTextFont = 2;
constexpr std::uint8_t kTextShade1 = 120;
constexpr std::uint8_t kTextShade2 = 124;
constexpr int kTextSpacing = 2;
constexpr int kTextX = 50, kTextY = 391;
constexpr int kLineStep = 13;
constexpr int kTextWidth = 295;

} // namespace

RaceSelectScreen::RaceSelectScreen(GameData& data, const Races& races, int pointerX, int pointerY)
    : races_(races), background_(Image::parse(data.archive(kBackgroundArchive).entry(kBackgroundEntry))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))),
      title_(Image::parse(data.archive(kArchive).entry(kTitleEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto& archive = data.archive(kArchive);
    for (std::size_t i = 0; i < kButtons; ++i) {
        buttons_.push_back(Image::parse(archive.entry(kFirstButton + i)));
        if (buttons_.back().frameCount() < 2) {
            throw FormatError("race button without its lit frame");
        }
        portraits_.push_back(Image::parse(archive.entry(kFirstPortrait + i)));
    }
    highlight(buttonAt(pointerX, pointerY).value_or(0));
}

Rect RaceSelectScreen::buttonRect(std::size_t button) const {
    const auto& img = buttons_.at(button);
    return {kColumnX[button / kRows], kFirstRowY + kRowStep * static_cast<int>(button % kRows), img.width(),
            img.height()};
}

std::optional<std::size_t> RaceSelectScreen::buttonAt(int x, int y) const {
    for (std::size_t i = 0; i < kButtons; ++i) {
        if (buttonRect(i).contains(x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

void RaceSelectScreen::highlight(std::size_t button) {
    if (button == highlighted_ && !lines_.empty()) {
        return;
    }
    highlighted_ = button;
    lines_.clear();
    if (button < Races::kStockRaces) {
        lines_ = wrapWords(fonts_.font(kTextFont), races_.description(races_.stock(button)), kTextWidth,
                           kTextSpacing);
    }
}

void RaceSelectScreen::pointerMoved(int x, int y) {
    if (const auto at = buttonAt(x, y)) {
        highlight(*at);
    }
}

void RaceSelectScreen::press(int x, int y) {
    pointerMoved(x, y);
    held_ = true;
}

std::optional<std::size_t> RaceSelectScreen::release(int x, int y) {
    pointerMoved(x, y);
    if (!held_) {
        return std::nullopt;
    }
    held_ = false;
    return buttonAt(x, y);
}

std::optional<std::size_t> RaceSelectScreen::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

void RaceSelectScreen::moveHighlight(int step) {
    const auto n = static_cast<int>(kButtons);
    highlight(static_cast<std::size_t>(((static_cast<int>(highlighted_) + step) % n + n) % n));
}

void RaceSelectScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    background_.draw(0, canvas);
    // The portrait goes under the panel, whose frame covers its edges.
    const Image& portrait = portraits_[highlighted_];
    portrait.applyPalette(palette);
    portrait.draw(0, canvas, kPortraitX, kPortraitY);
    panel_.draw(0, canvas, kPanelX, kPanelY);
    title_.draw(0, canvas, kTitleX, kTitleY);
    for (std::size_t i = 0; i < kButtons; ++i) {
        const Rect r = buttonRect(i);
        buttons_[i].draw(0, canvas, r.x, r.y);
        if (i == highlighted_) {
            buttons_[i].draw(1, canvas, r.x, r.y);
        }
    }
    TextStyle style;
    style.shades[1] = kTextShade1;
    style.shades[2] = kTextShade2;
    style.shadow = std::uint8_t{0};
    style.shadowAt = {{1, 0}, {0, 1}, {1, 1}};
    style.spacing = kTextSpacing;
    const Font& font = fonts_.font(kTextFont);
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        font.draw(canvas, kTextX, kTextY + kLineStep * static_cast<int>(i), lines_[i], style);
    }
}

} // namespace hegemon
