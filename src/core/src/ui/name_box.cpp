// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/name_box.hpp"

#include "hegemon/game_data.hpp"

#include <array>
#include <numeric>
#include <utility>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/ruler-name.md), from the box's top
// left corner.
constexpr const char* kArchive = "INBOX.LBX";
constexpr std::size_t kEntry = 0;
constexpr Rect kAccept = {96, 100, 98, 28};
constexpr std::size_t kTitleFont = 4;
constexpr int kTitleY = 22;
constexpr std::size_t kNameFont = 3;
constexpr int kNameX = 44, kNameY = 61;
constexpr int kSpacing = 2;

} // namespace

NameBox::NameBox(GameData& data, std::string title, std::string name, int x, int y, Shades shades)
    : picture_(Image::parse(data.archive(kArchive).entry(kEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), title_(std::move(title)), name_(std::move(name)),
      x_(x), y_(y), shades_(shades) {
    std::iota(colours_.begin(), colours_.end(), std::uint8_t{0});
    if (name_.size() > kMaxLength) {
        name_.resize(kMaxLength);
    }
}

bool NameBox::type(char c) {
    const auto u = static_cast<unsigned char>(c);
    const Font& font = fonts_.font(kNameFont);
    if (u < 32 || font.width(u) == 0 || name_.size() >= kMaxLength || font.textWidth(name_, kSpacing) >= kMaxWidth) {
        return false;
    }
    name_.push_back(c);
    return true;
}

void NameBox::backspace() {
    if (!name_.empty()) {
        name_.pop_back();
    }
}

Rect NameBox::acceptRect() const { return {x_ + kAccept.x, y_ + kAccept.y, kAccept.w, kAccept.h}; }

void NameBox::press(int, int) { held_ = true; }

bool NameBox::release(int x, int y) {
    const bool was = std::exchange(held_, false);
    return was && acceptRect().contains(x, y);
}

bool NameBox::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

void NameBox::tick(std::uint64_t ms) {
    // The original redraws the cursor as fast as it can, showing it every
    // other time; Hegemon flips it every tick of the PC timer.
    if (timer_.advance(ms) % 2 == 1) {
        cursorShown_ = !cursorShown_;
    }
}

void NameBox::draw(Canvas& canvas) const {
    picture_.draw(0, canvas, x_, y_, colours_);

    TextStyle title;
    for (std::size_t i = 0; i < shades_.title.size(); ++i) {
        title.shades[i + 1] = colours_[shades_.title[i]];
    }
    title.spacing = kSpacing;
    const Font& titleFont = fonts_.font(kTitleFont);
    const int titleX = x_ + (picture_.width() - titleFont.textWidth(title_, kSpacing)) / 2;
    titleFont.draw(canvas, titleX, y_ + kTitleY, title_, title);

    TextStyle name;
    for (std::size_t i = 0; i < shades_.name.size(); ++i) {
        name.shades[i + 1] = colours_[shades_.name[i]];
    }
    name.spacing = kSpacing;
    const Font& nameFont = fonts_.font(kNameFont);
    const int end = nameFont.draw(canvas, x_ + kNameX, y_ + kNameY, name_, name);
    if (cursorShown_) {
        nameFont.draw(canvas, end, y_ + kNameY, "_", name);
    }
}

} // namespace hegemon
