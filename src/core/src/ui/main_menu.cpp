// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/main_menu.hpp"

#include <string>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/main-menu.md).
constexpr const char* kArchive = "MAINMENU.LBX";
constexpr std::size_t kBackgroundEntry = 1;
constexpr std::size_t kPanelEntry = 2;
constexpr std::size_t kFirstButtonEntry = 3;
constexpr int kButtonX = 415;
constexpr std::array<int, MainMenu::kButtons> kButtonY = {172, 195, 217, 240, 262, 285};

constexpr int kNormal = 0;
constexpr int kHighlighted = 1;
constexpr int kGreyedOut = 2;

} // namespace

MainMenu::MainMenu(GameData& data)
    : background_(Image::parse(data.archive(kArchive).entry(kBackgroundEntry))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))) {
    const auto& archive = data.archive(kArchive);
    for (std::size_t b = 0; b < kButtons; ++b) {
        std::array<Image, 3> states;
        for (std::size_t s = 0; s < 3; ++s) {
            states[s] = Image::parse(archive.entry(kFirstButtonEntry + 3 * b + s));
        }
        buttons_.push_back(std::move(states));
    }
    // Continue needs the autosave in slot 10; Load needs a game in slots 1 to 9.
    bool anySave = false;
    for (int slot = 1; slot <= 9; ++slot) {
        anySave = anySave || data.hasFile("SAVE" + std::to_string(slot) + ".GAM");
    }
    enabled_.fill(true);
    enabled_[index(Choice::Continue)] = data.hasFile("SAVE10.GAM");
    enabled_[index(Choice::Load)] = anySave;
}

MainMenu::Rect MainMenu::buttonRect(std::size_t i) const {
    const auto& img = buttons_.at(i)[kNormal];
    return {kButtonX, kButtonY.at(i), img.width(), img.height()};
}

std::optional<std::size_t> MainMenu::buttonAt(int x, int y) const {
    for (std::size_t i = 0; i < kButtons; ++i) {
        if (buttonRect(i).contains(x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

void MainMenu::pointerMoved(int x, int y) {
    const auto at = buttonAt(x, y);
    highlighted_.reset();
    if (at && enabled_[*at]) {
        highlighted_ = static_cast<Choice>(*at);
    }
}

std::optional<MainMenu::Choice> MainMenu::click(int x, int y) {
    pointerMoved(x, y);
    return highlighted_;
}

void MainMenu::moveHighlight(int step) {
    std::size_t at = highlighted_ ? index(*highlighted_) : (step > 0 ? kButtons - 1 : 0);
    for (std::size_t tries = 0; tries < kButtons; ++tries) {
        at = (at + kButtons + static_cast<std::size_t>(step > 0 ? 1 : kButtons - 1)) % kButtons;
        if (enabled_[at]) {
            highlighted_ = static_cast<Choice>(at);
            return;
        }
    }
}

std::optional<MainMenu::Choice> MainMenu::activate() const {
    return highlighted_;
}

void MainMenu::draw(Canvas& canvas, Palette& palette) const {
    background_.applyPalette(palette);
    panel_.applyPalette(palette);
    background_.draw(0, canvas);
    panel_.draw(0, canvas);
    for (std::size_t b = 0; b < kButtons; ++b) {
        int state = kNormal;
        if (!enabled_[b]) {
            state = kGreyedOut;
        } else if (highlighted_ && index(*highlighted_) == b) {
            state = kHighlighted;
        }
        buttons_[b][static_cast<std::size_t>(state)].draw(0, canvas, kButtonX, kButtonY[b]);
    }
}

} // namespace hegemon
