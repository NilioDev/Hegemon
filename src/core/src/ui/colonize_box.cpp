// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/colonize_box.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <algorithm>
#include <array>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/colonizing.md): CONFIRM.LBX's box
// (0) at (161, 117), Yes (1) and No (2); the title in font 3 with two
// pixels between letters, centred on x 316 (left = 316 − (width − 2) / 2)
// at y 155; the rows in the same font from (204, 185), 15 pixels apart,
// their values ending at x 428; shades 1 and 2 in the box's colours 85 and
// 91.
constexpr const char* kArchive = "CONFIRM.LBX";
constexpr int kBoxX = 161, kBoxY = 117;
constexpr std::size_t kFont = 3;
constexpr int kSpacing = 2;
constexpr int kTitleCentre = 316, kTitleY = 155;
constexpr int kRowX = 204, kRowY = 185, kRowStep = 15, kValueEnd = 429;
constexpr std::uint8_t kShade1 = 85, kShade2 = 91;
// ESTRINGS.LBX: the title, then the rows, each "%s<words><spaces>%s<value>".
constexpr std::size_t kTitle = 209;
constexpr std::size_t kFoodHalf = 100, kFood = 101, kIndustry = 107, kMaintenance = 108, kMaxPopulation = 109,
                      kResearch = 115, kWorkerPenalty = 120;
// HESTRNGS.LBX: the word for a star's first planet.
constexpr std::size_t kPrime = 160;

// A row's words and value: the message split at its second %s.
std::pair<std::string, std::string> row(const std::string& message, long long value) {
    std::string words, rest;
    const auto first = message.find("%s");
    const auto second = first == std::string::npos ? std::string::npos : message.find("%s", first + 2);
    if (second == std::string::npos) {
        return {formatMessage(message, {value}), std::string()};
    }
    words = message.substr(first + 2, second - first - 2);
    while (!words.empty() && words.back() == ' ') {
        words.pop_back();
    }
    rest = formatMessage(message.substr(second + 2), {value});
    // Some of the messages end in a line break.
    while (!rest.empty() && (rest.back() == '\n' || rest.back() == '\r' || rest.back() == ' ')) {
        rest.pop_back();
    }
    return {words, rest};
}

} // namespace

ColonizeBox::ColonizeBox(GameData& data, const Game& game, int player, int planet)
    : planet_(planet), fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    for (std::size_t i = 0; i < 3; ++i) {
        pictures_.push_back(Image::parse(data.archive(kArchive).entry(i)));
    }
    const auto messages = parseStringList(data.archive("ESTRINGS.LBX").entry(0));
    const auto strings = parseStringList(data.archive("HESTRNGS.LBX").entry(0));
    if (messages.size() <= kTitle || strings.size() <= kPrime) {
        throw FormatError("too few messages in ESTRINGS.LBX or HESTRNGS.LBX");
    }
    title_ = formatMessage(messages[kTitle], {planetName(game.galaxy, planet, strings[kPrime])});
    while (!title_.empty() && (title_.back() == ' ' || title_.back() == '\n' || title_.back() == '\r')) {
        title_.pop_back();
    }
    const Player& p = game.players.at(static_cast<std::size_t>(player));
    const Planet& where = game.galaxy.planets.at(static_cast<std::size_t>(planet));
    const Economy economy(data.parameters());
    const int food2 = economy.farmerFood2(p, where);
    rows_.push_back(row(messages[food2 % 2 != 0 ? kFoodHalf : kFood], food2 / 2));
    rows_.push_back(row(messages[kIndustry], economy.workerProduction(p, where)));
    rows_.push_back(row(messages[kResearch], (economy.scientistResearch100(p, where) + 50) / 100));
    // Hegemon has no maintenance or gravity penalties yet.
    rows_.push_back(row(messages[kMaintenance], 0));
    rows_.push_back(row(messages[kWorkerPenalty], 0));
    rows_.push_back(row(messages[kMaxPopulation], economy.maxPopulation(p, where)));
}

void ColonizeBox::press(int x, int y) {
    held_ = kYes.contains(x, y) ? kYes : kNo.contains(x, y) ? kNo : Rect{};
}

ColonizeBox::Answer ColonizeBox::release(int x, int y) {
    const Rect held = held_;
    held_ = {};
    if (held.w == 0 || !held.contains(x, y)) {
        return Answer::None;
    }
    return held.x == kYes.x ? Answer::Yes : Answer::No;
}

void ColonizeBox::draw(Canvas& canvas, const Palette& palette) const {
    Palette own = palette;
    pictures_[0].applyPalette(own);
    const ColourMap colours = nearestColours(own, palette);
    pictures_[0].draw(0, canvas, kBoxX, kBoxY, colours);
    pictures_[1].draw(held_.w != 0 && held_.x == kYes.x ? 1 : 0, canvas, kYes.x, kYes.y, colours);
    pictures_[2].draw(held_.w != 0 && held_.x == kNo.x ? 1 : 0, canvas, kNo.x, kNo.y, colours);
    TextStyle style;
    style.spacing = kSpacing;
    style.shades[1] = colours[kShade1];
    style.shades[2] = style.shades[3] = colours[kShade2];
    const Font& font = fonts_.font(kFont);
    font.draw(canvas, kTitleCentre - (font.textWidth(title_, kSpacing) - kSpacing) / 2, kTitleY, title_, style);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const int y = kRowY + kRowStep * static_cast<int>(i);
        font.draw(canvas, kRowX, y, rows_[i].first, style);
        const std::string& value = rows_[i].second;
        font.draw(canvas, kValueEnd - (font.textWidth(value, kSpacing) - kSpacing), y, value, style);
    }
}

} // namespace hegemon
