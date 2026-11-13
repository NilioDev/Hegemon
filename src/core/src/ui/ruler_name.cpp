// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/ruler_name.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"
#include "hegemon/rules/races.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/ruler-name.md).
constexpr const char* kNamesArchive = "RACENAME.LBX";
constexpr const char* kTextArchive = "KENTEXT1.LBX"; // English
constexpr std::size_t kTitleEntry = 156;
constexpr const char* kBackgroundArchive = "RACEOPT.LBX";
constexpr std::size_t kBackgroundEntry = 0;
constexpr const char* kPortraitArchive = "RACESEL.LBX";
constexpr std::size_t kFirstPortrait = 15;
constexpr const char* kDialogArchive = "INBOX.LBX";
constexpr std::size_t kDialogEntry = 0;
constexpr int kDialogX = 177, kDialogY = 125;
constexpr Rect kAccept = {273, 225, 98, 28};

// The texts, in the dialog's own colours (they are drawn in the nearest
// colours of the screen, like the dialog).
constexpr std::size_t kTitleFont = 4;
constexpr int kTitleY = 147;
constexpr std::array<std::uint8_t, 3> kTitleShades = {10, 26, 20};
constexpr std::size_t kNameFont = 3;
constexpr int kNameX = 221, kNameY = 186;
constexpr std::array<std::uint8_t, 2> kNameShades = {0, 20};
constexpr int kSpacing = 2;

} // namespace

RulerNames::RulerNames(GameData& data) {
    const auto table = TextTable::parse(data.archive(kNamesArchive).entry(0));
    if (table.size() < Races::kStockRaces * kPerRace) {
        throw FormatError("too few ruler names");
    }
    for (std::size_t i = 0; i < Races::kStockRaces * kPerRace; ++i) {
        names_.push_back(table.at(i));
    }
}

const std::string& RulerNames::name(std::size_t race, std::size_t choice) const {
    if (race >= Races::kStockRaces || choice >= kPerRace) {
        throw std::out_of_range("no such ruler name");
    }
    return names_[race * kPerRace + choice];
}

RulerNameDialog::RulerNameDialog(GameData& data, std::size_t race, std::string name)
    : background_(Image::parse(data.archive(kBackgroundArchive).entry(kBackgroundEntry))),
      dialog_(Image::parse(data.archive(kDialogArchive).entry(kDialogEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), name_(std::move(name)) {
    if (race > Races::kStockRaces) {
        throw std::out_of_range("no such race button");
    }
    const Image portrait = Image::parse(data.archive(kPortraitArchive).entry(kFirstPortrait + race));
    const auto title = TextTable::parse(data.archive(kTextArchive).entry(kTitleEntry));
    if (title.size() < 1) {
        throw FormatError("the ruler name dialog has no title");
    }
    title_ = title.at(0);
    // The screen keeps the colours of the race selection with the race's
    // portrait; the dialog, made for other colours, takes the nearest ones.
    background_.applyPalette(palette_);
    portrait.applyPalette(palette_);
    Palette own;
    dialog_.applyPalette(own);
    colours_ = nearestColours(own, palette_);
    if (name_.size() > kMaxLength) {
        name_.resize(kMaxLength);
    }
}

bool RulerNameDialog::type(char c) {
    const auto u = static_cast<unsigned char>(c);
    const Font& font = fonts_.font(kNameFont);
    if (u < 32 || font.width(u) == 0 || name_.size() >= kMaxLength ||
        font.textWidth(name_, kSpacing) >= kMaxWidth) {
        return false;
    }
    name_.push_back(c);
    return true;
}

void RulerNameDialog::backspace() {
    if (!name_.empty()) {
        name_.pop_back();
    }
}

Rect RulerNameDialog::acceptRect() const { return kAccept; }

void RulerNameDialog::press(int, int) { held_ = true; }

bool RulerNameDialog::release(int x, int y) {
    const bool was = std::exchange(held_, false);
    return was && kAccept.contains(x, y);
}

bool RulerNameDialog::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

void RulerNameDialog::tick(std::uint64_t ms) {
    // The original redraws the cursor as fast as it can, showing it every
    // other time; Hegemon flips it every tick of the PC timer.
    if (timer_.advance(ms) % 2 == 1) {
        cursorShown_ = !cursorShown_;
    }
}

void RulerNameDialog::draw(Canvas& canvas, Palette& palette) const {
    palette = palette_;
    background_.draw(0, canvas);
    dialog_.draw(0, canvas, kDialogX, kDialogY, colours_);

    TextStyle title;
    for (std::size_t i = 0; i < kTitleShades.size(); ++i) {
        title.shades[i + 1] = colours_[kTitleShades[i]];
    }
    title.spacing = kSpacing;
    const Font& titleFont = fonts_.font(kTitleFont);
    const int titleX = kDialogX + (dialog_.width() - titleFont.textWidth(title_, kSpacing)) / 2;
    titleFont.draw(canvas, titleX, kTitleY, title_, title);

    TextStyle name;
    for (std::size_t i = 0; i < kNameShades.size(); ++i) {
        name.shades[i + 1] = colours_[kNameShades[i]];
    }
    name.spacing = kSpacing;
    const Font& nameFont = fonts_.font(kNameFont);
    const int end = nameFont.draw(canvas, kNameX, kNameY, name_, name);
    if (cursorShown_) {
        nameFont.draw(canvas, end, kNameY, "_", name);
    }
}

} // namespace hegemon
