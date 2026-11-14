// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/ui/research_done.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_data.hpp"

#include <cstdio>

namespace hegemon {

namespace {

// Measured on the original (docs/rules/research.md).
constexpr const char* kArchive = "SCIENCE.LBX";
constexpr std::size_t kColoursEntry = 0, kPanelEntry = 1, kBeamEntry = 2;
// The help texts: a record for each application, by its number, with its
// name at the start and its description from byte 103 on.
constexpr const char* kHelpArchive = "HELP.LBX";
constexpr std::size_t kDescriptionAt = 103;
// "Your scientists have completed their research in <field>!", the field's
// name in place of byte 0x89. Entry 11 holds the same text.
constexpr const char* kMessageArchive = "BILLTEXT.LBX";
constexpr std::size_t kMessageEntry = 6;
constexpr char kFieldMark = '\x89';
// The texts: centred on x 310 (the name in font 3 at y 45, the description
// in font 2 from y 59, 13 pixels a line) and x 337 (the message in font 4
// from y 420, 17 a line), 2 pixels between letters. How wide a line may get
// is only known to lie between what was seen and the next word: assumed.
constexpr int kPanelCentre = 310, kTitleY = 45, kDescriptionY = 59, kDescriptionStep = 13;
constexpr int kMessageCentre = 337, kMessageY = 420, kMessageStep = 17;
constexpr int kDescriptionWidth = 180, kMessageWidth = 380;
constexpr std::size_t kTitleFont = 3, kDescriptionFont = 2, kMessageFont = 4;
constexpr int kSpacing = 2;
constexpr std::uint8_t kEdge = 240, kGreen = 246;
constexpr std::uint8_t kMessageEdge = 242, kMessageGreen = 245, kMessageMid = 244;
// The message has a black border a pixel wide all around its letters.
constexpr std::uint8_t kMessageBorder = 0;

std::string scientistArchive(int race) {
    char name[16];
    std::snprintf(name, sizeof name, "SR_R%X_SC.LBX", static_cast<unsigned>(race) & 0xFU);
    return name;
}

std::string itemArchive(int tech) {
    char name[16];
    std::snprintf(name, sizeof name, "TANM_%03d.LBX", static_cast<unsigned>(tech) % 1000U);
    return name;
}

// The frame after `frame`: past the last the animation starts again at the
// frame its header names (the panel goes back to 5), or at the first when
// that is the last frame itself (the scientist).
std::size_t nextFrame(const Image& image, std::size_t frame) {
    if (frame + 1 < image.frameCount()) {
        return frame + 1;
    }
    const std::size_t loop = image.loopFrame();
    return loop + 1 < image.frameCount() ? loop : 0;
}

void drawUpTo(const Image& image, std::size_t frame, Canvas& canvas) {
    for (std::size_t k = 0; k <= frame && k < image.frameCount(); ++k) {
        image.draw(k, canvas);
    }
}

void drawCentred(const Font& font, Canvas& canvas, int centre, int y, const std::string& text,
                 const TextStyle& style) {
    const int w = font.textWidth(text, style.spacing);
    font.draw(canvas, centre - (w + 1) / 2, y, text, style);
}

} // namespace

ResearchDoneScreen::ResearchDoneScreen(GameData& data, const TechTree& tree, int race, int field, int tech)
    : scientist_(Image::parse(data.archive(scientistArchive(race)).entry(0))),
      beam_(Image::parse(data.archive(kArchive).entry(kBeamEntry))),
      item_(Image::parse(data.archive(itemArchive(tech)).entry(0))),
      panel_(Image::parse(data.archive(kArchive).entry(kPanelEntry))),
      colours_(Image::parse(data.archive(kArchive).entry(kColoursEntry))),
      fonts_(FontSet::parse(data.archive("FONTS.LBX").entry(0))), scientistCanvas_(640, 480), scene_(640, 480) {
    for (const Image* image : {&scientist_, &beam_, &item_, &panel_}) {
        if (image->frameCount() == 0) {
            throw FormatError("research done screen: an animation has no pictures");
        }
    }
    const auto help = data.archive(kHelpArchive).entry(0);
    const TextTable names = TextTable::parse(help);
    const TextTable texts = TextTable::parse(help, kDescriptionAt);
    if (tech <= 0 || static_cast<std::size_t>(tech) >= names.size()) {
        throw FormatError("HELP.LBX: no text for the application");
    }
    title_ = names.at(static_cast<std::size_t>(tech));
    description_ = wrapWords(fonts_.font(kDescriptionFont), texts.at(static_cast<std::size_t>(tech)),
                             kDescriptionWidth, kSpacing);

    const TechNames techNames(data.archive("TECHNAME.LBX").entry(0), tree);
    const TextTable bill = TextTable::parse(data.archive(kMessageArchive).entry(kMessageEntry));
    if (bill.size() == 0) {
        throw FormatError("BILLTEXT.LBX: the message is missing");
    }
    std::string message;
    for (const char c : bill.at(0)) {
        if (c == kFieldMark) {
            message += techNames.field(field);
        } else {
            message.push_back(c);
        }
    }
    message_ = wrapWords(fonts_.font(kMessageFont), message, kMessageWidth, kSpacing);

    scientist_.draw(0, scientistCanvas_);
    compose();
}

void ResearchDoneScreen::compose() {
    scene_ = scientistCanvas_;
    drawUpTo(beam_, frames_[1], scene_);
    drawUpTo(item_, frames_[2], scene_);
    drawUpTo(panel_, frames_[3], scene_);
}

void ResearchDoneScreen::tick(std::uint64_t ms) {
    const std::uint64_t ticks = timer_.advance(ms);
    if (ticks == 0) {
        return;
    }
    for (std::uint64_t t = 0; t < ticks; ++t) {
        frames_[0] = nextFrame(scientist_, frames_[0]);
        scientist_.draw(frames_[0], scientistCanvas_);
        frames_[1] = nextFrame(beam_, frames_[1]);
        frames_[2] = nextFrame(item_, frames_[2]);
        frames_[3] = nextFrame(panel_, frames_[3]);
    }
    compose();
}

void ResearchDoneScreen::draw(Canvas& canvas, Palette& palette) const {
    palette = Palette{};
    scientist_.applyPalette(palette);
    item_.applyPalette(palette);
    colours_.applyPalette(palette);
    for (int y = 0; y < canvas.height() && y < scene_.height(); ++y) {
        for (int x = 0; x < canvas.width() && x < scene_.width(); ++x) {
            canvas.set(x, y, scene_.at(x, y));
        }
    }
    TextStyle panel;
    panel.shades[1] = kEdge;
    panel.shades[2] = panel.shades[3] = kGreen;
    panel.spacing = kSpacing;
    drawCentred(fonts_.font(kTitleFont), canvas, kPanelCentre, kTitleY, title_, panel);
    for (std::size_t i = 0; i < description_.size(); ++i) {
        drawCentred(fonts_.font(kDescriptionFont), canvas, kPanelCentre,
                    kDescriptionY + kDescriptionStep * static_cast<int>(i), description_[i], panel);
    }
    TextStyle bottom;
    bottom.shades[1] = kMessageEdge;
    bottom.shades[2] = kMessageGreen;
    bottom.shades[3] = kMessageMid;
    bottom.spacing = kSpacing;
    bottom.shadow = kMessageBorder;
    bottom.shadowAt = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    for (std::size_t i = 0; i < message_.size(); ++i) {
        drawCentred(fonts_.font(kMessageFont), canvas, kMessageCentre, kMessageY + kMessageStep * static_cast<int>(i),
                    message_[i], bottom);
    }
}

} // namespace hegemon
