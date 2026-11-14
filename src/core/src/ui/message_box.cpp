// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/message_box.hpp"

#include "hegemon/game_data.hpp"

#include <algorithm>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colony-screen.md).
// A message: TEXTBOX.LBX's top (0), as much of its body (1) as the lines
// need, 15 pixels a line less one, and its bottom (2), centred on the
// screen at x 130; the text in font 3 from x 159, 33 pixels below the top,
// 15 a line, shades 166 and 168 of its colours; lines broken to fit 322 pixels; Close (3)
// 16 pixels into the bottom at x 288.
constexpr const char* kMessageArchive = "TEXTBOX.LBX";
constexpr int kMessageX = 130, kMessageTextX = 159, kMessageTextY = 33, kMessageLine = 15, kMessageWidth = 322;
constexpr int kTopHeight = 42, kBottomHeight = 43, kCloseX = 288, kCloseY = 16;
constexpr std::size_t kMessageFont = 3;
constexpr int kStripeTop = 9, kStripeBottom = 9, kStripePhase = 11, kStripeLeft = 140, kStripeRight = 500;
constexpr std::uint8_t kMessageShade1 = 166, kMessageShade2 = 168;
// A question: CONFIRM.LBX's box (0) at (161, 117), Yes (1) and No (2); the
// text in font 4 centred on 317, 17 pixels a line, the block centred on
// 216, shades 230, 232 and 231 of its colours; lines broken to fit 230 pixels.
constexpr const char* kQuestionArchive = "CONFIRM.LBX";
constexpr int kQuestionX = 161, kQuestionY = 117, kQuestionCentre = 317, kQuestionLine = 17, kQuestionMiddle = 216;
constexpr int kQuestionWidth = 230;
constexpr std::size_t kQuestionFont = 4;
constexpr std::uint8_t kQuestionShade1 = 230, kQuestionShade2 = 232, kQuestionShade3 = 231;
constexpr int kSpacing = 2;
constexpr int kScreenHeight = 480;

std::vector<std::string> wrap(const Font& font, const std::string& text, int width) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t at = 0;
    while (at < text.size()) {
        std::size_t end = text.find(' ', at);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string word = text.substr(at, end - at);
        const std::string longer = line.empty() ? word : line + " " + word;
        if (!line.empty() && font.textWidth(longer, kSpacing) > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = longer;
        }
        at = end + 1;
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

MessageBox::MessageBox(GameData& data, Kind kind, const std::string& text)
    : kind_(kind), fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const char* archive = kind == Kind::Message ? kMessageArchive : kQuestionArchive;
    const std::size_t pictures = kind == Kind::Message ? 4 : 3;
    for (std::size_t i = 0; i < pictures; ++i) {
        pictures_.push_back(Image::parse(data.archive(archive).entry(i)));
    }
    if (kind == Kind::Message) {
        lines_ = wrap(fonts_.font(kMessageFont), text, kMessageWidth);
    } else {
        lines_ = wrap(fonts_.font(kQuestionFont), text, kQuestionWidth);
    }
}

int MessageBox::bodyHeight() const {
    return std::max(0, kMessageLine * static_cast<int>(lines_.size()) - 1);
}

int MessageBox::top() const {
    return (kScreenHeight - kTopHeight - bodyHeight() - kBottomHeight) / 2;
}

Rect MessageBox::closeRect() const {
    const Image& close = pictures_[3];
    return {kCloseX, top() + kTopHeight + bodyHeight() + kCloseY, close.width(), close.height()};
}

void MessageBox::press(int x, int y) {
    held_ = {};
    if (kind_ == Kind::Message) {
        if (closeRect().contains(x, y)) {
            held_ = closeRect();
        }
    } else if (kYes.contains(x, y)) {
        held_ = kYes;
    } else if (kNo.contains(x, y)) {
        held_ = kNo;
    }
}

MessageBox::Answer MessageBox::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    if (held.w == 0 || !held.contains(x, y)) {
        return Answer::None;
    }
    if (kind_ == Kind::Message) {
        return Answer::Close;
    }
    return held.x == kYes.x ? Answer::Yes : Answer::No;
}

void MessageBox::draw(Canvas& canvas, const Palette& palette) const {
    // Measured: the box keeps the screen's colours and is drawn in those
    // nearest to its own.
    Palette own = palette;
    pictures_[0].applyPalette(own);
    const ColourMap colours = nearestColours(own, palette);
    TextStyle style;
    style.spacing = kSpacing;
    if (kind_ == Kind::Message) {
        const int y = top();
        const int bottom = y + kTopHeight + bodyHeight();
        // Measured: the inside is lines of the box's colours 2, 3 and 4 in
        // turn, the first 11 rows down, from x 140 to 499 and down to 9 rows
        // into the bottom picture; the pictures are drawn over them.
        for (int row = y + kStripeTop; row < bottom + kStripeBottom; ++row) {
            const std::uint8_t colour = colours[static_cast<std::size_t>(2 + (row - y - kStripePhase + 3) % 3)];
            for (int col = kStripeLeft; col < kStripeRight; ++col) {
                if (row >= 0 && row < canvas.height() && col < canvas.width()) {
                    canvas.set(col, row, colour);
                }
            }
        }
        pictures_[0].draw(0, canvas, kMessageX, y, colours);
        // The body's top rows only: its sides.
        const Image& body = pictures_[1];
        Canvas a(body.width(), body.height(), 0), b(body.width(), body.height(), 1);
        body.draw(0, a);
        body.draw(0, b);
        for (int row = 0; row < bodyHeight() && row < body.height(); ++row) {
            for (int col = 0; col < body.width(); ++col) {
                const int cx = kMessageX + col, cy = y + kTopHeight + row;
                if (a.at(col, row) == b.at(col, row) && cx < canvas.width() && cy < canvas.height()) {
                    canvas.set(cx, cy, colours[a.at(col, row)]);
                }
            }
        }
        pictures_[2].draw(0, canvas, kMessageX, y + kTopHeight + bodyHeight(), colours);
        const Rect close = closeRect();
        pictures_[3].draw(held_.w != 0 ? 1 : 0, canvas, close.x, close.y, colours);
        style.shades[1] = colours[kMessageShade1];
        style.shades[2] = style.shades[3] = colours[kMessageShade2];
        const Font& font = fonts_.font(kMessageFont);
        for (std::size_t k = 0; k < lines_.size(); ++k) {
            font.draw(canvas, kMessageTextX, y + kMessageTextY + kMessageLine * static_cast<int>(k), lines_[k], style);
        }
        return;
    }
    pictures_[0].draw(0, canvas, kQuestionX, kQuestionY, colours);
    pictures_[1].draw(held_.w != 0 && held_.x == kYes.x ? 1 : 0, canvas, kYes.x, kYes.y, colours);
    pictures_[2].draw(held_.w != 0 && held_.x == kNo.x ? 1 : 0, canvas, kNo.x, kNo.y, colours);
    style.shades[1] = colours[kQuestionShade1];
    style.shades[2] = colours[kQuestionShade2];
    style.shades[3] = colours[kQuestionShade3];
    const Font& font = fonts_.font(kQuestionFont);
    const int first = kQuestionMiddle - kQuestionLine * static_cast<int>(lines_.size()) / 2;
    for (std::size_t k = 0; k < lines_.size(); ++k) {
        font.draw(canvas, kQuestionCentre - font.textWidth(lines_[k], kSpacing) / 2,
                  first + kQuestionLine * static_cast<int>(k), lines_[k], style);
    }
}

} // namespace hegemon
