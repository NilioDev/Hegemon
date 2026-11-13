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
constexpr int kDialogX = 177, kDialogY = 125;
// The texts' colours, in the box's own colours (drawn, like the box, in
// the nearest colours of the screen).
constexpr NameBox::Shades kShades = {{10, 26, 20}, {0, 20}};

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
      box_(data, title(data), std::move(name), kDialogX, kDialogY, kShades) {
    if (race > Races::kStockRaces) {
        throw std::out_of_range("no such race button");
    }
    const Image portrait = Image::parse(data.archive(kPortraitArchive).entry(kFirstPortrait + race));
    // The screen keeps the colours of the race selection with the race's
    // portrait; the dialog, made for other colours, takes the nearest ones.
    background_.applyPalette(palette_);
    portrait.applyPalette(palette_);
    Palette own;
    box_.picture().applyPalette(own);
    box_.useColours(nearestColours(own, palette_));
}

std::string RulerNameDialog::title(GameData& data) {
    const auto title = TextTable::parse(data.archive(kTextArchive).entry(kTitleEntry));
    if (title.size() < 1) {
        throw FormatError("the ruler name dialog has no title");
    }
    return title.at(0);
}

void RulerNameDialog::draw(Canvas& canvas, Palette& palette) const {
    palette = palette_;
    background_.draw(0, canvas);
    box_.draw(canvas);
}

} // namespace hegemon
