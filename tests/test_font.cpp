// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/font.hpp"
#include "hegemon/data/lbx.hpp"
#include "hegemon/data/pointers.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/data/text_table.hpp"
#include "hegemon/game_files.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

using hegemon::Canvas;
using hegemon::FontSet;
using hegemon::FormatError;
using hegemon::TextStyle;

namespace {

constexpr std::size_t kHeights = 0x56C;
constexpr std::size_t kWidths = 0x59C;
constexpr std::size_t kOffsets = 0xB9C;
constexpr std::size_t kGlyphs = kOffsets + 6 * 1024;

void put32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8 * i));
    }
}

// A font entry whose font 0 has two characters: 'A', 3 pixels wide, and 'B',
// 2 wide. Every glyph offset of every font points past 'B'.
std::vector<std::uint8_t> makeFonts() {
    // 'A': row 0 is shade 1, skip 1, shade 2; row 1 is skip 1, shade 1.
    const std::vector<std::uint8_t> a = {0x01, 0x81, 0x02, 0x80, 0x81, 0x01};
    // 'B': a 2×1 bar of shade 2, then a row with too many pixels.
    const std::vector<std::uint8_t> b = {0x02, 0x02, 0x80, 0x01, 0x01, 0x01, 0x01};
    std::vector<std::uint8_t> out(kGlyphs, 0);
    out[kHeights] = 2;
    out[kWidths + 'A'] = 3;
    out[kWidths + 'B'] = 2;
    const auto end = static_cast<std::uint32_t>(a.size() + b.size());
    for (std::size_t f = 0; f < FontSet::kFonts; ++f) {
        for (std::size_t c = 0; c < 256; ++c) {
            put32(out, kOffsets + 1024 * f + 4 * c, end);
        }
    }
    put32(out, kOffsets + 4 * 'A', 0);
    put32(out, kOffsets + 4 * 'B', static_cast<std::uint32_t>(a.size()));
    out.insert(out.end(), a.begin(), a.end());
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

TextStyle plainStyle() {
    TextStyle style;
    style.shades[1] = std::uint8_t{10};
    style.shades[2] = std::uint8_t{20};
    return style;
}

} // namespace

HGM_TEST("Fonts: heights, widths and glyph rows with skips") {
    const auto set = FontSet::parse(makeFonts());
    const auto& font = set.font(0);
    HGM_CHECK(font.height() == 2);
    HGM_CHECK(font.width('A') == 3);
    HGM_CHECK(font.width('B') == 2);
    HGM_CHECK(font.width('C') == 0);
    HGM_CHECK(font.textWidth("AB", 1) == 7);

    Canvas canvas(8, 4, 99);
    const int end = font.draw(canvas, 1, 1, "AB", plainStyle());
    HGM_CHECK(end == 8);
    HGM_CHECK(canvas.at(1, 1) == 10);
    HGM_CHECK(canvas.at(2, 1) == 99);
    HGM_CHECK(canvas.at(3, 1) == 20);
    HGM_CHECK(canvas.at(2, 2) == 10);
    HGM_CHECK(canvas.at(5, 1) == 20);
    HGM_CHECK(canvas.at(6, 1) == 20);
    // The second row of 'B' is cut at the glyph's width.
    HGM_CHECK(canvas.at(5, 2) == 10);
    HGM_CHECK(canvas.at(6, 2) == 10);
    HGM_CHECK(canvas.at(7, 2) == 99);
}

HGM_TEST("Fonts: shadows go under the glyphs and stay inside the clip") {
    const auto set = FontSet::parse(makeFonts());
    const auto& font = set.font(0);
    TextStyle style = plainStyle();
    style.shadow = std::uint8_t{0};

    // The usual shadow: one pixel right of and below every drawn pixel.
    Canvas plain(6, 4, 99);
    font.draw(plain, 0, 0, "A", style);
    HGM_CHECK(plain.at(0, 0) == 10);
    HGM_CHECK(plain.at(1, 0) == 99);
    HGM_CHECK(plain.at(1, 1) == 10); // the glyph goes over the shadow of (0, 0)
    HGM_CHECK(plain.at(3, 1) == 0);
    HGM_CHECK(plain.at(2, 2) == 0);

    // An outline all round, clipped on the left and at the top.
    style.shadowAt = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    Canvas outlined(6, 5, 99);
    font.draw(outlined, 1, 1, "A", style, {1, 1, 6, 3});
    HGM_CHECK(outlined.at(0, 1) == 99);
    HGM_CHECK(outlined.at(1, 0) == 99);
    HGM_CHECK(outlined.at(2, 1) == 0);
    HGM_CHECK(outlined.at(4, 1) == 0);
    HGM_CHECK(outlined.at(1, 2) == 0);
    HGM_CHECK(outlined.at(2, 2) == 10);
    HGM_CHECK(outlined.at(2, 3) == 99); // below the clip
}

HGM_TEST("Fonts: damaged entries are refused") {
    auto entry = makeFonts();
    entry.resize(kGlyphs - 1);
    bool threw = false;
    try {
        (void)FontSet::parse(entry);
    } catch (const FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);

    entry = makeFonts();
    put32(entry, kOffsets + 4 * 'A', 0x10000);
    threw = false;
    try {
        (void)FontSet::parse(entry);
    } catch (const FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);
}

HGM_TEST("Text tables: fixed-size records cut at their first zero") {
    const std::vector<std::uint8_t> entry = {2, 0, 4, 0, 'a', 'b', 0, 'x', 'c', 'd', 'e', 'f'};
    const auto table = hegemon::TextTable::parse(entry);
    HGM_CHECK(table.size() == 2);
    HGM_CHECK(table.at(0) == "ab");
    HGM_CHECK(table.at(1) == "cdef");
    // A text further into each record.
    const auto second = hegemon::TextTable::parse(entry, 1);
    HGM_CHECK(second.at(0) == "b" && second.at(1) == "def");

    bool threw = false;
    try {
        (void)hegemon::TextTable::parse(std::vector<std::uint8_t>{3, 0, 4, 0, 'a', 'b', 'c', 'd'});
    } catch (const FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);
    threw = false;
    try {
        (void)hegemon::TextTable::parse(entry, 4);
    } catch (const FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);
}

HGM_TEST("Fonts: the player's FONTS.LBX and IFONTS.LBX read without errors") {
    const std::filesystem::path game = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(game)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    for (const char* name : {"FONTS.LBX", "IFONTS.LBX"}) {
        const auto path = hegemon::findFileNoCase(game, name);
        HGM_CHECK(path.has_value());
        if (!path) {
            continue;
        }
        const auto archive = hegemon::LbxArchive::load(*path);
        const auto set = FontSet::parse(archive.entry(0));
        for (std::size_t f = 0; f < FontSet::kFonts; ++f) {
            HGM_CHECK(set.font(f).height() > 0);
            HGM_CHECK(set.font(f).width('A') > 0);
        }
    }
}

HGM_TEST("Pointers: 24×24 after a palette, colour 0 see-through, hot spot top left") {
    std::vector<std::uint8_t> entry(1024 + 20 * 24 * 24, 0);
    entry[1024 + 576 + 0] = 7;           // pointer 1, (0, 0)
    entry[1024 + 576 + 2 * 24 + 23] = 9; // pointer 1, (23, 2)
    const auto set = hegemon::PointerSet::parse(entry);
    Canvas canvas(40, 40, 99);
    set.draw(1, canvas, 10, 5);
    HGM_CHECK(canvas.at(10, 5) == 7);
    HGM_CHECK(canvas.at(33, 7) == 9);
    HGM_CHECK(canvas.at(11, 5) == 99);
    set.draw(1, canvas, 30, 38); // clipped at the edges
    HGM_CHECK(canvas.at(30, 38) == 7);

    entry.pop_back();
    bool threw = false;
    try {
        (void)hegemon::PointerSet::parse(entry);
    } catch (const FormatError&) {
        threw = true;
    }
    HGM_CHECK(threw);
}
