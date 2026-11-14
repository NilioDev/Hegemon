// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/research_select.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_data.hpp"

#include <cstdio>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/research.md).
constexpr const char* kArchive = "TECHSEL.LBX";
constexpr std::size_t kPanelEntry = 0;
constexpr int kPanelX = 161;
// Carries the colours the texts are drawn in (224 to 239).
constexpr std::size_t kTextColoursEntry = 14;
constexpr std::size_t kNamesEntry = 0; // of TECHNAME.LBX
// The boxes: two columns, four rows.
constexpr int kBoxX = 174, kColumnStep = 227, kBoxW = 218, kBoxH = 82;
constexpr std::array<int, 4> kBoxY = {48, 153, 258, 365};
// The field's name, 3 pixels into the box (one more in the top right box),
// in font 4; the applications in font 3 below it, 15 pixels apart; the
// cost in font 3 above the box, ending 2 pixels before its right edge.
constexpr int kNameDX = 2, kNameDY = 3;
constexpr int kAppDX = 12, kAppDY = 19, kAppStep = 15;
constexpr int kCostRight = 215, kCostDY = -19;
constexpr std::size_t kNameFont = 4, kTextFont = 3;
constexpr int kSpacing = 2;
constexpr std::uint8_t kDark = 225, kGreen = 228, kLitGreen = 231;
// The lit box's border and the arrow to the application under the pointer,
// in two colours the original sets itself, taking turns (how fast is
// assumed). Hegemon keeps them in colours the screen doesn't use.
constexpr std::uint8_t kBorder = 250, kBorderBright = 251;
constexpr Color kBorderColour = {125, 219, 20}, kBorderBrightColour = {158, 251, 28};
constexpr std::uint64_t kBlinkTicks = 4;
// The arrow: down from 19 pixels into the box, 3 from its left edge, then
// right to the application, its head 7 pixels high.
constexpr int kArrowDX = 3, kArrowTopDY = 19, kArrowLength = 7, kArrowAppDY = 5;
// The scientist's animation: a frame every two ticks (assumed).
constexpr std::uint64_t kTicksPerFrame = 2;

std::string scientistArchive(int race) {
    char name[16];
    std::snprintf(name, sizeof name, "SR_R%X_SC.LBX", static_cast<unsigned>(race) & 0xFU);
    return name;
}

// Green letters with dark edges, lit when the box is.
TextStyle textStyle(bool lit) {
    TextStyle style;
    style.shades[1] = kDark;
    style.shades[2] = style.shades[3] = lit ? kLitGreen : kGreen;
    style.spacing = kSpacing;
    return style;
}

void line(Canvas& c, int x0, int y0, int x1, int y1, std::uint8_t colour) {
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (x >= 0 && y >= 0 && x < c.width() && y < c.height()) {
                c.set(x, y, colour);
            }
        }
    }
}

} // namespace

ResearchSelectScreen::ResearchSelectScreen(GameData& data, const TechTree& tree, const Knowledge& knowledge, int race,
                                           int pointerX, int pointerY)
    : scientist_(Image::parse(data.archive(scientistArchive(race)).entry(0))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))),
      textColours_(Image::parse(data.archive(kArchive).entry(kTextColoursEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), animation_(640, 480) {
    if (scientist_.frameCount() == 0) {
        throw FormatError("research screen: the scientist has no pictures");
    }
    const auto colours = data.archive("FONTS.LBX").entry(kPointerEntry);
    if (colours.size() < 1024) {
        throw FormatError("FONTS.LBX: the research screen's colours are missing");
    }
    base_.set(0, colours.subspan(0, 1024));
    const TechNames names(data.archive("TECHNAME.LBX").entry(kNamesEntry), tree);
    for (std::size_t i = 0; i < kBoxes; ++i) {
        Box& b = boxes_[i];
        b.field = tree.researchable(knowledge, kOrder[i]);
        if (b.field == 0) {
            continue;
        }
        const TechField& f = tree.field(b.field);
        b.name = names.field(f.id);
        b.cost = std::to_string(f.cost) + " RP";
        for (const int t : f.techs) {
            b.apps.push_back(names.tech(t));
        }
    }
    scientist_.draw(0, animation_);
    pointerMoved(pointerX, pointerY);
}

Rect ResearchSelectScreen::boxRect(std::size_t box) const {
    return {kBoxX + kColumnStep * static_cast<int>(box % 2), kBoxY.at(box / 2), kBoxW, kBoxH};
}

int ResearchSelectScreen::nameY(std::size_t box) const {
    // Measured: the top right box's texts are a pixel lower.
    return boxRect(box).y + kNameDY + (box == 1 ? 1 : 0);
}

std::optional<std::size_t> ResearchSelectScreen::boxAt(int x, int y) const {
    for (std::size_t i = 0; i < kBoxes; ++i) {
        if (boxRect(i).contains(x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

void ResearchSelectScreen::pointerMoved(int x, int y) {
    highlighted_ = boxAt(x, y);
    app_.reset();
    if (highlighted_ && boxes_[*highlighted_].field == 0) {
        highlighted_.reset();
    }
    if (highlighted_) {
        const int first = nameY(*highlighted_) + kAppDY;
        const std::size_t apps = boxes_[*highlighted_].apps.size();
        if (y >= first && y < first + kAppStep * static_cast<int>(apps)) {
            app_ = static_cast<std::size_t>((y - first) / kAppStep);
        }
    }
}

void ResearchSelectScreen::press(int x, int y) {
    pointerMoved(x, y);
    held_ = true;
}

std::optional<int> ResearchSelectScreen::release(int x, int y) {
    pointerMoved(x, y);
    if (!held_) {
        return std::nullopt;
    }
    held_ = false;
    return choose();
}

std::optional<int> ResearchSelectScreen::click(int x, int y) {
    press(x, y);
    return release(x, y);
}

std::optional<int> ResearchSelectScreen::choose() const {
    if (!highlighted_ || boxes_[*highlighted_].field == 0) {
        return std::nullopt;
    }
    return boxes_[*highlighted_].field;
}

void ResearchSelectScreen::moveHighlight(int step) {
    const auto n = static_cast<int>(kBoxes);
    int at = highlighted_ ? static_cast<int>(*highlighted_) : (step > 0 ? -1 : 0);
    for (int tries = 0; tries < n; ++tries) {
        at = ((at + step) % n + n) % n;
        if (boxes_[static_cast<std::size_t>(at)].field != 0) {
            highlighted_ = static_cast<std::size_t>(at);
            app_.reset();
            return;
        }
    }
}

void ResearchSelectScreen::tick(std::uint64_t ms) {
    const std::uint64_t ticks = timer_.advance(ms);
    for (std::uint64_t t = 0; t < ticks; ++t) {
        ++ticks_;
        if (ticks_ % kTicksPerFrame == 0) {
            frame_ = frame_ + 1 < scientist_.frameCount() ? frame_ + 1 : 0;
            scientist_.draw(frame_, animation_);
        }
    }
}

void ResearchSelectScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = base_;
    scientist_.applyPalette(palette);
    panel_.applyPalette(palette);
    textColours_.applyPalette(palette);
    palette.colors[kBorder] = kBorderColour;
    palette.colors[kBorderBright] = kBorderBrightColour;
    for (int y = 0; y < canvas.height() && y < animation_.height(); ++y) {
        for (int x = 0; x < canvas.width() && x < animation_.width(); ++x) {
            canvas.set(x, y, animation_.at(x, y));
        }
    }
    panel_.draw(0, canvas, kPanelX, 0);

    const Font& nameFont = fonts_.font(kNameFont);
    const Font& textFont = fonts_.font(kTextFont);
    for (std::size_t i = 0; i < kBoxes; ++i) {
        const Box& b = boxes_[i];
        if (b.field == 0) {
            continue;
        }
        const bool lit = highlighted_ == i;
        const Rect r = boxRect(i);
        const TextStyle style = textStyle(lit);
        const TextStyle appStyle = textStyle(false);
        const int y = nameY(i);
        nameFont.draw(canvas, r.x + kNameDX, y, b.name, style);
        textFont.draw(canvas, r.x + kCostRight - textFont.textWidth(b.cost, kSpacing), y + kCostDY, b.cost, style);
        for (std::size_t k = 0; k < b.apps.size(); ++k) {
            textFont.draw(canvas, r.x + kAppDX, y + kAppDY + kAppStep * static_cast<int>(k), b.apps[k],
                          lit && app_ == k ? style : appStyle);
        }
        if (!lit) {
            continue;
        }
        const std::uint8_t c = (ticks_ / kBlinkTicks) % 2 == 0 ? kBorderBright : kBorder;
        line(canvas, r.x, r.y, r.x + r.w - 1, r.y, c);
        line(canvas, r.x, r.y + r.h - 1, r.x + r.w - 1, r.y + r.h - 1, c);
        line(canvas, r.x, r.y, r.x, r.y + r.h - 1, c);
        line(canvas, r.x + r.w - 1, r.y, r.x + r.w - 1, r.y + r.h - 1, c);
        if (app_) {
            const int ax = r.x + kArrowDX;
            const int ay = y + kAppDY + kAppStep * static_cast<int>(*app_) + kArrowAppDY;
            line(canvas, ax, r.y + kArrowTopDY, ax, ay, c);
            line(canvas, ax, ay, ax + kArrowLength, ay, c);
            for (int d = 1; d <= 3; ++d) {
                line(canvas, ax + kArrowLength - d, ay - d, ax + kArrowLength - d, ay - d, c);
                line(canvas, ax + kArrowLength - d, ay + d, ax + kArrowLength - d, ay + d, c);
            }
        }
    }
}

} // namespace hegemon
