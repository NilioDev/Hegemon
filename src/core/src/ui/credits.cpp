// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/credits.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <charconv>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/main-menu.md).
constexpr std::size_t kFont = 3;
constexpr int kLeftX = 66;
constexpr int kRightX = 194;
constexpr int kLeftMaxWidth = 126; // the left text and its dots
// The window the credits show through, in rows of the screen (the bottom
// one not included), and the left edge no shadow goes past.
constexpr int kWindowTop = 179;
constexpr int kWindowBottom = 423;
// Once the last line's top is above this row, the credits start again.
constexpr int kRestartAbove = 180;
constexpr int kStartTop = 456;
constexpr int kLineHeight = 14;
constexpr int kStep = 2;          // pixels per step
constexpr std::uint64_t kTicksPerStep = 3;

// Near the top and the bottom of the window the lines fade: the colours of
// their two shades by where the line's top is.
struct Fade {
    int fromTop; // this colour pair for tops from here down
    std::uint8_t shade1, shade2;
};
constexpr std::array<Fade, 9> kFades = {{
    {std::numeric_limits<int>::min(), 212, 211},
    {172, 212, 213},
    {174, 212, 215},
    {178, 212, 217},
    {180, 213, 220}, // not faded
    {410, 211, 219},
    {412, 211, 217},
    {416, 211, 215},
    {420, 211, 213},
}};

TextStyle creditsStyle(int top) {
    TextStyle style;
    const Fade* fade = &kFades.front();
    for (const auto& f : kFades) {
        if (top >= f.fromTop) {
            fade = &f;
        }
    }
    style.shades[1] = fade->shade1;
    style.shades[2] = fade->shade2;
    style.shadow = std::uint8_t{0};
    // An outline all round each pixel.
    style.shadowAt = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    style.spacing = 1;
    return style;
}

} // namespace

Credits::Credits(GameData& data) : fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto table = TextTable::parse(data.archive("CREDITS.LBX").entry(0));
    if (table.size() == 0) {
        throw FormatError("no credits");
    }
    // The first record holds the number of lines; then each line is two
    // records, the left and the right column.
    std::size_t count = 0;
    const auto& first = table.at(0);
    std::from_chars(first.data(), first.data() + first.size(), count);
    count = std::min(count, (table.size() - 1) / 2);
    for (std::size_t i = 0; i < count; ++i) {
        lines_.emplace_back(table.at(1 + 2 * i), table.at(2 + 2 * i));
    }
}

int Credits::lineTop(std::size_t i) const {
    return kStartTop + kLineHeight * static_cast<int>(i) - offset_;
}

void Credits::advanceTicks(std::uint64_t ticks) {
    for (std::uint64_t t = 0; t < ticks; ++t) {
        ++ticksDone_;
        if (ticksDone_ % kTicksPerStep != 0) {
            continue;
        }
        offset_ += kStep;
        // Once the last line has gone past the top, they start again.
        if (lines_.empty() || lineTop(lines_.size() - 1) < kRestartAbove) {
            offset_ = 0;
        }
    }
}

void Credits::tick(std::uint64_t ms) {
    advanceTicks(timer_.advance(ms));
}

void Credits::draw(Canvas& canvas) const {
    const Font& font = fonts_.font(kFont);
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const int top = lineTop(i);
        if (top + kLineHeight <= kWindowTop || top >= kWindowBottom) {
            continue;
        }
        // Each line is drawn on its own: its shadow never reaches above its
        // top, nor left of the left column.
        const TextClip clip{kLeftX, std::max(top, kWindowTop), canvas.width(), kWindowBottom};
        const TextStyle style = creditsStyle(top);
        const auto& [left, right] = lines_[i];
        if (!left.empty()) {
            std::string text = left + " ";
            const int dot = font.textWidth(".", style.spacing);
            int width = font.textWidth(text, style.spacing);
            while (width + dot <= kLeftMaxWidth) {
                text.push_back('.');
                width += dot;
            }
            font.draw(canvas, kLeftX, top, text, style, clip);
        }
        font.draw(canvas, kRightX, top, right, style, clip);
    }
}

void Credits::restart() {
    offset_ = 0;
    ticksDone_ = 0;
    timer_ = PitTimer{};
}

} // namespace hegemon
