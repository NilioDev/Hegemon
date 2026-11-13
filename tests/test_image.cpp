// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/image.hpp"
#include "hegemon/data/lbx.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <vector>

using hegemon::Canvas;
using hegemon::FormatError;
using hegemon::Image;
using hegemon::Palette;

namespace {

void add16(std::vector<std::uint8_t>& out, unsigned v) {
    out.push_back(static_cast<std::uint8_t>(v));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
}

void add32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    add16(out, v & 0xFFFF);
    add16(out, v >> 16);
}

// Builds an entry with one frame from its header values, an optional palette
// part and the frame's bytes.
std::vector<std::uint8_t> makeEntry(unsigned w, unsigned h, unsigned flags, const std::vector<std::uint8_t>& palette,
                                    const std::vector<std::uint8_t>& frame) {
    std::vector<std::uint8_t> out;
    for (unsigned v : {w, h, 0u, 1u, 0u, flags}) {
        add16(out, v);
    }
    const auto start = static_cast<std::uint32_t>(12 + 8 + palette.size());
    add32(out, start);
    add32(out, start + static_cast<std::uint32_t>(frame.size()));
    out.insert(out.end(), palette.begin(), palette.end());
    out.insert(out.end(), frame.begin(), frame.end());
    return out;
}

// A 4×3 frame: line 0 gets pixels 7, 8, 9 from x = 1; line 2 gets 5 at x = 0
// and, after a gap of two, 6 at x = 3.
std::vector<std::uint8_t> sampleFrame() {
    std::vector<std::uint8_t> f;
    add16(f, 1);
    add16(f, 0);
    add16(f, 3);
    add16(f, 1);
    f.insert(f.end(), {7, 8, 9, 0});
    add16(f, 0);
    add16(f, 2);
    add16(f, 1);
    add16(f, 0);
    f.insert(f.end(), {5, 0});
    add16(f, 1);
    add16(f, 2);
    f.insert(f.end(), {6, 0});
    add16(f, 0);
    add16(f, 1000);
    return f;
}

template <typename F>
bool throwsFormatError(F&& f) {
    try {
        f();
    } catch (const FormatError&) {
        return true;
    }
    return false;
}

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

HGM_TEST("Image: runs land where they belong, gaps stay transparent") {
    const auto image = Image::parse(makeEntry(4, 3, 0, {}, sampleFrame()));
    HGM_CHECK(image.width() == 4);
    HGM_CHECK(image.height() == 3);
    HGM_CHECK(image.frameCount() == 1);
    HGM_CHECK(!image.paletteChange());
    Canvas canvas(4, 3, 1);
    image.draw(0, canvas);
    HGM_CHECK(canvas.at(0, 0) == 1);
    HGM_CHECK(canvas.at(1, 0) == 7);
    HGM_CHECK(canvas.at(3, 0) == 9);
    HGM_CHECK(canvas.at(2, 1) == 1);
    HGM_CHECK(canvas.at(0, 2) == 5);
    HGM_CHECK(canvas.at(1, 2) == 1);
    HGM_CHECK(canvas.at(2, 2) == 1);
    HGM_CHECK(canvas.at(3, 2) == 6);

    // Drawn partly off the canvas: only what fits is drawn.
    Canvas small(2, 2, 0);
    image.draw(0, small, -1, 0);
    HGM_CHECK(small.at(0, 0) == 7);
    HGM_CHECK(small.at(1, 0) == 8);
}

HGM_TEST("Image: palette part and plain rows") {
    std::vector<std::uint8_t> palette;
    add16(palette, 10);
    add16(palette, 2);
    palette.insert(palette.end(), {0, 63, 0, 32, 0, 1, 2, 3});
    std::vector<std::uint8_t> raw = {1, 2, 3, 4, 5, 6, 0, 0};
    const auto image = Image::parse(makeEntry(3, 2, Image::kFlagRaw | Image::kFlagPalette, palette, raw));
    HGM_CHECK(image.paletteChange().has_value());
    Palette pal;
    image.applyPalette(pal);
    HGM_CHECK((pal.colors[10] == hegemon::Color{255, 0, 130}));
    HGM_CHECK((pal.colors[11] == hegemon::Color{4, 8, 12}));
    HGM_CHECK((pal.colors[12] == hegemon::Color{0, 0, 0}));
    Canvas canvas(3, 2);
    image.draw(0, canvas);
    HGM_CHECK(canvas.at(0, 0) == 1);
    HGM_CHECK(canvas.at(2, 1) == 6);

    std::vector<std::uint8_t> rgba(3 * 2 * 4);
    pal.colors[6] = {9, 8, 7};
    hegemon::toRgba(canvas, pal, rgba);
    HGM_CHECK(rgba[20] == 9);
    HGM_CHECK(rgba[23] == 255);
}

HGM_TEST("Image: damaged pictures are refused") {
    // A run past the right edge.
    auto frame = sampleFrame();
    HGM_CHECK(throwsFormatError([&] { (void)Image::parse(makeEntry(3, 3, 0, {}, frame)); }));
    // A run below the last line.
    HGM_CHECK(throwsFormatError([&] { (void)Image::parse(makeEntry(4, 2, 0, {}, frame)); }));
    // No end marker.
    frame.resize(frame.size() - 4);
    HGM_CHECK(throwsFormatError([&] { (void)Image::parse(makeEntry(4, 3, 0, {}, frame)); }));
    // Plain rows shorter than the picture.
    HGM_CHECK(throwsFormatError([] { (void)Image::parse(makeEntry(3, 2, Image::kFlagRaw, {}, {1, 2, 3})); }));
    // A palette part past colour 255.
    std::vector<std::uint8_t> palette;
    add16(palette, 255);
    add16(palette, 2);
    palette.resize(palette.size() + 8);
    HGM_CHECK(throwsFormatError([&] { (void)Image::parse(makeEntry(4, 3, Image::kFlagPalette, palette, sampleFrame())); }));
    // Cut short anywhere.
    const auto whole = makeEntry(4, 3, 0, {}, sampleFrame());
    for (std::size_t n = 0; n < whole.size(); ++n) {
        const std::vector<std::uint8_t> cut(whole.begin(), whole.begin() + static_cast<std::ptrdiff_t>(n));
        HGM_CHECK(throwsFormatError([&] { (void)Image::parse(cut); }));
    }
}

// With the player's files: the main menu picture and its colours, and every
// picture in every archive.
HGM_TEST("Image: the original's pictures") {
    const std::filesystem::path folder = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    const auto menu = hegemon::LbxArchive::load(*hegemon::findFileNoCase(folder, "MAINMENU.LBX"));
    const auto picture = Image::parse(menu.entry(1));
    HGM_CHECK(picture.width() == 640);
    HGM_CHECK(picture.height() == 480);
    HGM_CHECK(picture.paletteChange() && picture.paletteChange()->colors.size() == 256);
    const auto opening = Image::parse(menu.entry(0));
    HGM_CHECK(opening.frameCount() == 50);

    std::size_t pictures = 0;
    std::size_t raw = 0;
    for (const auto& file : std::filesystem::directory_iterator(folder)) {
        auto bytes = readFile(file.path());
        if (!hegemon::LbxArchive::looksLikeLbx(bytes)) {
            continue;
        }
        const auto archive = hegemon::LbxArchive::parse(std::move(bytes));
        for (std::size_t i = 0; i < archive.size(); ++i) {
            try {
                const auto image = Image::parse(archive.entry(i));
                ++pictures;
                raw += (image.flags() & Image::kFlagRaw) != 0 ? 1 : 0;
            } catch (const FormatError&) {
                // Not a picture: a sound, a text or a table.
            }
        }
    }
    HGM_CHECK(pictures >= 6532);
    HGM_CHECK(raw == 43);
}
