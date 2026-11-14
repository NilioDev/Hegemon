// SPDX-License-Identifier: GPL-3.0-or-later
//
// The screen that tells the player their scientists have finished a field:
// the race scientist, the application learnt turning in a beam of light and
// its name and description on a glass panel, drawn as the original draws
// it. See docs/rules/research.md.
#pragma once

#include "hegemon/data/font.hpp"
#include "hegemon/data/image.hpp"
#include "hegemon/rules/tech.hpp"
#include "hegemon/ui/pit_timer.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hegemon {

class GameData;

class ResearchDoneScreen {
public:
    // Reads the pictures and texts for the race (0 to 12, as Races counts
    // them), the field finished and the application learnt in it. Throws
    // FormatError when a file is damaged or has nothing for the
    // application.
    ResearchDoneScreen(GameData& data, const TechTree& tree, int race, int field, int tech);

    // The texts as drawn: the application's name, its description and the
    // message at the bottom, each broken into lines.
    const std::string& title() const { return title_; }
    const std::vector<std::string>& description() const { return description_; }
    const std::vector<std::string>& message() const { return message_; }

    // The frame each animation shows: scientist, beam, application, panel.
    std::array<std::size_t, 4> frames() const { return frames_; }

    // Lets `ms` milliseconds pass: the animations move a frame each tick.
    void tick(std::uint64_t ms);

    // Draws the whole screen and sets the colours it is drawn with.
    void draw(Canvas& canvas, Palette& palette) const;

private:
    void compose();

    Image scientist_; // the race's animation, a whole screen
    Image beam_;
    Image item_; // the application, a whole screen
    Image panel_;
    Image colours_; // the entry that carries the beam's and texts' colours
    FontSet fonts_;
    std::string title_;
    std::vector<std::string> description_, message_;
    Canvas scientistCanvas_, scene_;
    std::array<std::size_t, 4> frames_{};
    PitTimer timer_;
};

} // namespace hegemon
