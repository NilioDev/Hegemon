// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/load_game.hpp"

#include "hegemon/game_data.hpp"

#include <array>
#include <cstdio>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/saved-games.md).
// The window: GAME.LBX 11 at (180, 51), drawn for the game's colours
// (FONTS.LBX 1); Load (12) and Cancel (13), their second frame while held;
// the player picture (23), in the screen's own colours.
constexpr const char* kArchive = "GAME.LBX";
constexpr std::size_t kWindowEntry = 11, kLoadEntry = 12, kCancelEntry = 13, kPlayerEntry = 23;
constexpr std::size_t kGameColours = 1; // of FONTS.LBX
constexpr int kWindowX = 180, kWindowY = 51;
// The slots: 29 pixels high, one every 31 from y 71, from x 202, 232 wide.
constexpr int kSlotX = 202, kSlotY = 71, kSlotPitch = 31, kSlotW = 232, kSlotH = 29;
// In a slot: the name in font 2 at (212, +4); "Stardate:" in font 1 at
// (212, +18), the stardate at 254, the date at 312, the time at 367; the
// player picture at (408, +16).
constexpr int kTextX = 212, kNameY = 4, kLineY = 18, kStardateX = 254, kDateX = 312, kTimeX = 367;
constexpr int kPlayerX = 408, kPlayerY = 16;
constexpr std::size_t kNameFont = 2, kLineFont = 1;
constexpr int kNameSpacing = 2, kLineSpacing = 1;
// The main menu's colours: 254 and 255, 251 and 252 for the slot used last.
constexpr std::uint8_t kShade1 = 254, kShade2 = 255, kLastShade1 = 251, kLastShade2 = 252;

// The warning: WARNING.LBX 0 at (154, 144), seven frames of lights shown
// back and forth, one every two timer ticks; the text in font 4 centred on
// 322, 88 pixels down, shades 3, 137 and 123 of the screen's colours.
constexpr const char* kWarningArchive = "WARNING.LBX";
constexpr int kWarningX = 154, kWarningY = 144, kWarningCentre = 322, kWarningTextY = 88;
constexpr std::size_t kWarningFont = 4;
constexpr int kWarningSpacing = 2;
constexpr std::uint8_t kWarnShade1 = 3, kWarnShade2 = 137, kWarnShade3 = 123;
constexpr std::uint64_t kPitHz = 1193182, kPitDivisor = 65536, kTicksAFrame = 2;

constexpr std::array<const char*, 12> kMonths = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

} // namespace

std::string savedDate(int year, int month, int day) {
    const char* name = month >= 1 && month <= 12 ? kMonths[static_cast<std::size_t>(month - 1)] : "???";
    return std::string(name) + " " + std::to_string(day) + ", " + std::to_string(year);
}

std::string savedTime(int hour, int minute) {
    std::array<char, 16> b{};
    std::snprintf(b.data(), b.size(), "%02d:%02d", hour % 100, minute % 100);
    return b.data();
}

LoadGameWindow::LoadGameWindow(GameData& data, Slots slots, std::optional<std::size_t> last)
    : window_(Image::parse(data.archive(kArchive).entry(kWindowEntry))),
      load_(Image::parse(data.archive(kArchive).entry(kLoadEntry))),
      cancel_(Image::parse(data.archive(kArchive).entry(kCancelEntry))),
      player_(Image::parse(data.archive(kArchive).entry(kPlayerEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), slots_(std::move(slots)), last_(last) {
    const auto colours = data.archive("FONTS.LBX").entry(kGameColours);
    if (colours.size() < 1024) {
        throw FormatError("FONTS.LBX: the game's colours are missing");
    }
    own_.set(0, colours.subspan(0, 1024));
    if (last_ && *last_ >= kSlots) {
        last_.reset();
    }
}

Rect LoadGameWindow::slotRect(std::size_t slot) {
    return {kSlotX, kSlotY + kSlotPitch * static_cast<int>(slot), kSlotW, kSlotH};
}

LoadGameWindow::Result LoadGameWindow::press(int x, int y) {
    held_ = {};
    for (std::size_t s = 0; s < kSlots; ++s) {
        if (slotRect(s).contains(x, y)) {
            return {Result::Kind::Load, s};
        }
    }
    if (kLoad.contains(x, y)) {
        held_ = kLoad;
    } else if (kCancel.contains(x, y)) {
        held_ = kCancel;
    }
    return {};
}

LoadGameWindow::Result LoadGameWindow::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    if (held.w == 0 || !held.contains(x, y)) {
        return {};
    }
    if (held.x == kCancel.x) {
        return {Result::Kind::Cancel, 0};
    }
    if (last_) {
        return {Result::Kind::Load, *last_};
    }
    return {};
}

void LoadGameWindow::draw(Canvas& canvas, const Palette& palette) const {
    const ColourMap colours = nearestColours(own_, palette);
    window_.draw(0, canvas, kWindowX, kWindowY, colours);
    load_.draw(held_.w != 0 && held_.x == kLoad.x ? 1 : 0, canvas, kLoad.x, kLoad.y, colours);
    cancel_.draw(held_.w != 0 && held_.x == kCancel.x ? 1 : 0, canvas, kCancel.x, kCancel.y, colours);
    const Font& nameFont = fonts_.font(kNameFont);
    const Font& lineFont = fonts_.font(kLineFont);
    for (std::size_t s = 0; s < kSlots; ++s) {
        const int top = slotRect(s).y;
        const bool last = last_ == s;
        TextStyle style;
        style.shades[1] = last ? kLastShade1 : kShade1;
        style.shades[2] = last ? kLastShade2 : kShade2;
        style.spacing = kNameSpacing;
        if (!slots_[s]) {
            nameFont.draw(canvas, kTextX, top + kNameY, "... empty slot ...", style);
            continue;
        }
        const SaveSlot& slot = *slots_[s];
        nameFont.draw(canvas, kTextX, top + kNameY, slot.name, style);
        style.spacing = kLineSpacing;
        const int line = top + kLineY;
        lineFont.draw(canvas, kTextX, line, "Stardate:", style);
        lineFont.draw(canvas, kStardateX, line,
                      std::to_string(slot.stardate / 10) + "." + std::to_string(slot.stardate % 10), style);
        lineFont.draw(canvas, kDateX, line, slot.date, style);
        lineFont.draw(canvas, kTimeX, line, slot.time, style);
        player_.draw(0, canvas, kPlayerX, top + kPlayerY);
    }
}

WarningBox::WarningBox(GameData& data, std::string text)
    : box_(Image::parse(data.archive(kWarningArchive).entry(0))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), text_(std::move(text)) {
    if (box_.frameCount() == 0) {
        throw FormatError("WARNING.LBX: the warning box has no pictures");
    }
}

void WarningBox::advance(std::uint64_t ms) {
    micros_ += ms * 1000;
}

void WarningBox::draw(Canvas& canvas, const Palette& palette) const {
    Palette own = palette;
    box_.applyPalette(own);
    const ColourMap colours = nearestColours(own, palette);
    // Back and forth: 0, 1, ... 6, 5, ... 1, 0, ...
    const std::uint64_t frames = box_.frameCount();
    const std::uint64_t step = micros_ * kPitHz / (kPitDivisor * 1000000) / kTicksAFrame;
    const std::uint64_t period = frames > 1 ? 2 * (frames - 1) : 1;
    const std::uint64_t at = step % period;
    const std::size_t frame = static_cast<std::size_t>(at < frames ? at : period - at);
    box_.draw(frame, canvas, kWarningX, kWarningY, colours);
    TextStyle style;
    style.spacing = kWarningSpacing;
    style.shades[1] = kWarnShade1;
    style.shades[2] = kWarnShade2;
    style.shades[3] = kWarnShade3;
    const Font& font = fonts_.font(kWarningFont);
    font.draw(canvas, kWarningCentre - font.textWidth(text_, kWarningSpacing) / 2, kWarningY + kWarningTextY, text_,
              style);
}

} // namespace hegemon
