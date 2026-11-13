// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/new_game.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"

namespace hegemon {

namespace {

// Measured on the original (docs/rules/new-game.md).
constexpr const char* kArchive = "NEWGAME.LBX";
constexpr const char* kBackgroundArchive = "RACEOPT.LBX";
constexpr std::size_t kBackgroundEntry = 0;
constexpr std::size_t kPanelEntry = 31;
constexpr int kPanelX = 15;
constexpr int kPanelY = 5;
constexpr std::size_t kFirstPicture = 1;
constexpr std::size_t kPictures = 25;
constexpr std::size_t kFirstSwitch = 26;
constexpr std::size_t kCancelEntry = 29;
constexpr std::size_t kAcceptEntry = 30;

// Where the pictures go, with the centre and top of the text under each.
struct Slot {
    int x, y;         // the picture's top left corner
    int textX, textY; // the text's centre and top
};
constexpr std::array<Slot, 5> kSlots = {{
    {136, 124, 169, 214}, // difficulty
    {291, 124, 325, 214}, // galaxy size
    {446, 124, 481, 214}, // galaxy age
    {136, 264, 169, 359}, // players
    {291, 264, 325, 359}, // tech level
}};
constexpr int kPictureSize = 65;
// The box around a text, from its centre and top.
constexpr int kBoxLeft = 53, kBoxTop = 8, kBoxW = 107, kBoxH = 26;

constexpr std::array<std::array<int, 2>, 3> kSwitchAt = {{{395, 264}, {395, 300}, {395, 335}}};
constexpr std::array<std::array<int, 2>, 2> kButtonAt = {{{115, 391}, {433, 392}}};

// The texts: font 2, two pixels between letters, in two greens.
constexpr std::size_t kTextFont = 2;
constexpr std::uint8_t kTextShade1 = 121;
constexpr std::uint8_t kTextShade2 = 123;
constexpr int kTextSpacing = 2;

// First picture of each list (entries of NEWGAME.LBX), in click order.
constexpr std::size_t kDifficultyPictures = 4;
constexpr std::size_t kSizePictures = 9;
constexpr std::size_t kAgePictures = 1;
constexpr std::size_t kPlayerPictures = 14; // 2 to 8 players, then random
constexpr std::size_t kTechPictures = 22;

constexpr std::array<std::string_view, 5> kDifficultyTexts = {"Tutor", "Easy", "Average", "Hard", "Impossible"};
constexpr std::array<std::string_view, 5> kSizeTexts = {"Small", "Medium", "Large", "Cluster", "Huge"};
constexpr std::array<std::string_view, 3> kAgeTexts = {"Mineral Rich", "Average", "Organic Rich"};
constexpr std::array<std::string_view, 8> kPlayerTexts = {"2 Players", "3 Players", "4 Players", "5 Players",
                                                          "6 Players", "7 Players", "8 Players", "Random"};
constexpr std::array<std::string_view, 4> kTechTexts = {"Pre-warp", "Average", "Post-warp", "Advanced"};

template <typename E, std::size_t N>
E next(E value) {
    return static_cast<E>((static_cast<std::size_t>(value) + 1) % N);
}

std::size_t playerIndex(int players) {
    if (players < NewGameSettings::kMinPlayers || players > NewGameSettings::kMaxPlayers) {
        return 7; // random
    }
    return static_cast<std::size_t>(players - NewGameSettings::kMinPlayers);
}

} // namespace

NewGameScreen::NewGameScreen(GameData& data, const NewGameSettings& settings)
    : settings_(settings),
      background_(Image::parse(data.archive(kBackgroundArchive).entry(kBackgroundEntry))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))) {
    const auto& archive = data.archive(kArchive);
    for (std::size_t i = 0; i < kPictures; ++i) {
        pictures_.push_back(Image::parse(archive.entry(kFirstPicture + i)));
    }
    for (std::size_t i = 0; i < switches_.size(); ++i) {
        switches_[i] = Image::parse(archive.entry(kFirstSwitch + i));
        if (switches_[i].frameCount() < 2) {
            throw FormatError("new game switch without its lit frame");
        }
    }
    buttons_[0] = Image::parse(archive.entry(kCancelEntry));
    buttons_[1] = Image::parse(archive.entry(kAcceptEntry));
    for (const auto& b : buttons_) {
        if (b.frameCount() < 2) {
            throw FormatError("new game button without its pressed frame");
        }
    }
}

Rect NewGameScreen::itemRect(Item item) const {
    const auto i = static_cast<std::size_t>(item);
    if (item <= Item::TechLevel) {
        const Slot& s = kSlots[i];
        // The picture down to the bottom of the box under it.
        return {s.textX - kBoxLeft, s.y, kBoxW, s.textY - kBoxTop + kBoxH - s.y};
    }
    if (item <= Item::AntaransAttack) {
        const std::size_t n = i - static_cast<std::size_t>(Item::TacticalCombat);
        return {kSwitchAt[n][0], kSwitchAt[n][1], switches_[n].width(), switches_[n].height()};
    }
    const std::size_t n = item == Item::Cancel ? 0 : 1;
    return {kButtonAt[n][0], kButtonAt[n][1], buttons_[n].width(), buttons_[n].height()};
}

std::optional<NewGameScreen::Item> NewGameScreen::itemAt(int x, int y) const {
    for (std::size_t i = 0; i < kItems; ++i) {
        const auto item = static_cast<Item>(i);
        if (item <= Item::TechLevel) {
            // Only the picture and the box count, not the gap between them.
            const Slot& s = kSlots[i];
            const Rect picture{s.x, s.y, kPictureSize, kPictureSize};
            const Rect box{s.textX - kBoxLeft, s.textY - kBoxTop, kBoxW, kBoxH};
            if (picture.contains(x, y) || box.contains(x, y)) {
                return item;
            }
        } else if (itemRect(item).contains(x, y)) {
            return item;
        }
    }
    return std::nullopt;
}

void NewGameScreen::pointerMoved(int x, int y) {
    pointerX_ = x;
    pointerY_ = y;
}

void NewGameScreen::press(int x, int y) {
    pointerMoved(x, y);
    held_ = true;
    const auto item = itemAt(x, y);
    if (item == Item::TacticalCombat) {
        settings_.tacticalCombat = !settings_.tacticalCombat;
    } else if (item == Item::RandomEvents) {
        settings_.randomEvents = !settings_.randomEvents;
    } else if (item == Item::AntaransAttack) {
        settings_.antaransAttack = !settings_.antaransAttack;
    }
}

std::optional<NewGameScreen::Result> NewGameScreen::release(int x, int y) {
    pointerMoved(x, y);
    if (!held_) {
        return std::nullopt;
    }
    held_ = false;
    const auto item = itemAt(x, y);
    if (!item) {
        return std::nullopt;
    }
    if (*item == Item::Cancel) {
        return Result::Cancel;
    }
    if (*item == Item::Accept) {
        return Result::Accept;
    }
    if (*item <= Item::TechLevel) {
        step(*item);
    }
    return std::nullopt;
}

std::optional<NewGameScreen::Result> NewGameScreen::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

void NewGameScreen::step(Item item) {
    using S = NewGameSettings;
    switch (item) {
    case Item::Difficulty:
        settings_.difficulty = next<S::Difficulty, kDifficultyTexts.size()>(settings_.difficulty);
        break;
    case Item::GalaxySize:
        settings_.galaxySize = next<S::GalaxySize, kSizeTexts.size()>(settings_.galaxySize);
        break;
    case Item::GalaxyAge:
        settings_.galaxyAge = next<S::GalaxyAge, kAgeTexts.size()>(settings_.galaxyAge);
        break;
    case Item::Players: {
        const std::size_t at = (playerIndex(settings_.players) + 1) % kPlayerTexts.size();
        settings_.players = at + 1 == kPlayerTexts.size() ? S::kRandomPlayers
                                                          : S::kMinPlayers + static_cast<int>(at);
        break;
    }
    case Item::TechLevel:
        settings_.techLevel = next<S::TechLevel, kTechTexts.size()>(settings_.techLevel);
        break;
    default:
        break;
    }
}

std::string_view NewGameScreen::valueText(Item item) const {
    const auto at = [](auto value) { return static_cast<std::size_t>(value); };
    switch (item) {
    case Item::Difficulty:
        return kDifficultyTexts[at(settings_.difficulty) % kDifficultyTexts.size()];
    case Item::GalaxySize:
        return kSizeTexts[at(settings_.galaxySize) % kSizeTexts.size()];
    case Item::GalaxyAge:
        return kAgeTexts[at(settings_.galaxyAge) % kAgeTexts.size()];
    case Item::Players:
        return kPlayerTexts[playerIndex(settings_.players)];
    case Item::TechLevel:
        return kTechTexts[at(settings_.techLevel) % kTechTexts.size()];
    default:
        return {};
    }
}

std::size_t NewGameScreen::pictureEntry(Item item) const {
    const auto at = [](auto value) { return static_cast<std::size_t>(value); };
    switch (item) {
    case Item::Difficulty:
        return kDifficultyPictures + at(settings_.difficulty) % kDifficultyTexts.size();
    case Item::GalaxySize:
        return kSizePictures + at(settings_.galaxySize) % kSizeTexts.size();
    case Item::GalaxyAge:
        return kAgePictures + at(settings_.galaxyAge) % kAgeTexts.size();
    case Item::Players:
        return kPlayerPictures + playerIndex(settings_.players);
    default:
        return kTechPictures + at(settings_.techLevel) % kTechTexts.size();
    }
}

void NewGameScreen::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    background_.draw(0, canvas);
    panel_.draw(0, canvas, kPanelX, kPanelY);

    TextStyle style;
    style.shades[1] = kTextShade1;
    style.shades[2] = kTextShade2;
    style.spacing = kTextSpacing;
    const Font& font = fonts_.font(kTextFont);
    for (std::size_t i = 0; i < kSlots.size(); ++i) {
        const auto item = static_cast<Item>(i);
        const Image& picture = pictures_.at(pictureEntry(item) - kFirstPicture);
        // The galaxy age pictures carry the colours 128 to 255 for all of them.
        picture.applyPalette(palette);
        picture.draw(0, canvas, kSlots[i].x, kSlots[i].y);
        const std::string_view text = valueText(item);
        const int width = font.textWidth(text, style.spacing) - style.spacing;
        font.draw(canvas, kSlots[i].textX - width / 2, kSlots[i].textY, text, style);
    }

    const std::array<bool, 3> on = {settings_.tacticalCombat, settings_.randomEvents, settings_.antaransAttack};
    for (std::size_t i = 0; i < switches_.size(); ++i) {
        switches_[i].draw(0, canvas, kSwitchAt[i][0], kSwitchAt[i][1]);
        if (on[i]) {
            switches_[i].draw(1, canvas, kSwitchAt[i][0], kSwitchAt[i][1]); // the lit lamp
        }
    }
    const auto over = held_ ? itemAt(pointerX_, pointerY_) : std::nullopt;
    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        buttons_[i].draw(0, canvas, kButtonAt[i][0], kButtonAt[i][1]);
        if (over == (i == 0 ? Item::Cancel : Item::Accept)) {
            buttons_[i].draw(1, canvas, kButtonAt[i][0], kButtonAt[i][1]); // pressed
        }
    }
}

} // namespace hegemon
