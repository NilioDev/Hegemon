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

// The version of the fan patch Hegemon plays like, for $VERSION$.
constexpr const char* kVersion = "1.51";

constexpr int kNormal = 0;
constexpr int kHighlighted = 1;
constexpr int kGreyedOut = 2;

// A text label as the fan patch's `format` setting gives it.
MainMenu::Label readLabel(const FanPatchConfig* config, std::string_view id, MainMenu::Label label) {
    if (config == nullptr) {
        return label;
    }
    const auto set = [&](std::string_view property, const ConfigValue& v) {
        const bool number = v.kind == ConfigValue::Kind::Number;
        if (property == "template") {
            label.text = v.text;
        } else if (property == "style" && number && v.number >= 0 &&
                   v.number < static_cast<std::int64_t>(FontSet::kFonts)) {
            label.font = static_cast<std::size_t>(v.number);
        } else if (property == "align" && !v.text.empty()) {
            label.align = v.text[0];
        } else if (property == "x" && number) {
            label.x = static_cast<int>(v.number);
        } else if (property == "y" && number) {
            label.y = static_cast<int>(v.number);
        }
    };
    constexpr std::array<std::string_view, 5> kProperties = {"template", "style", "align", "x", "y"};
    for (const auto* e : config->startingWith({"format", id})) {
        if (e->keys.size() == 2) {
            for (std::size_t i = 0; i < e->values.size() && i < kProperties.size(); ++i) {
                set(kProperties[i], e->values[i]);
            }
        } else if (e->keys.size() == 3 && !e->values.empty()) {
            set(e->keys[2], e->values[0]);
        }
    }
    return label;
}

std::string expand(std::string text, std::string_view name, std::string_view value) {
    const std::string var = "$" + std::string(name) + "$";
    for (std::size_t at = 0; (at = text.find(var, at)) != std::string::npos; at += value.size()) {
        text.replace(at, var.size(), value);
    }
    return text;
}

} // namespace

MainMenu::MainMenu(GameData& data)
    : background_(Image::parse(data.archive(kArchive).entry(kBackgroundEntry))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))), credits_(data) {
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

    // The fan patch's defaults, unless its settings say otherwise.
    const auto* config = data.config();
    std::string modName;
    if (config != nullptr && !config->mods().empty()) {
        modName = config->mods().front().name;
    }
    version_ = readLabel(config, "version", {"Version: $VERSION$", 2, 'r', 590, 415});
    version_.text = expand(version_.text, "VERSION", kVersion);
    modName_ = readLabel(config, "mod_name", {"Config: $MOD$", 2, 'r', 590, 400});
    modName_.text = expand(modName_.text, "MOD", modName);
}

void MainMenu::drawLabel(Canvas& canvas, const Label& label) const {
    if (label.text.empty()) {
        return;
    }
    const Font& font = credits_.fonts().font(label.font);
    TextStyle style;
    style.shades[1] = 208;
    style.shades[2] = 215;
    // Right and centre align on the last pixel column of the text.
    const int width = font.textWidth(label.text, style.spacing) - style.spacing;
    int x = label.x;
    if (label.align == 'r') {
        x -= width;
    } else if (label.align == 'c') {
        x -= width / 2;
    }
    font.draw(canvas, x, label.y, label.text, style);
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
    credits_.draw(canvas);
    drawLabel(canvas, modName_);
    drawLabel(canvas, version_);
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
