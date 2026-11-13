// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/font.hpp"

#include "hegemon/data/reader.hpp"

namespace hegemon {

namespace {

// Where the tables sit in the entry (docs/formats/fonts.md).
constexpr std::size_t kHeights = 0x56C;
constexpr std::size_t kWidths = 0x59C;
constexpr std::size_t kOffsets = 0xB9C;
constexpr std::size_t kGlyphs = kOffsets + FontSet::kFonts * 256 * 4;
constexpr std::uint8_t kNewRow = 0x80;
constexpr std::size_t kMaxRows = 64;

} // namespace

FontSet FontSet::parse(std::span<const std::uint8_t> entry) {
    if (entry.size() < kGlyphs) {
        throw FormatError("font entry shorter than its tables");
    }
    FontSet set;
    Reader r(entry);
    for (std::size_t f = 0; f < kFonts; ++f) {
        Font& font = set.fonts_[f];
        r.seek(kHeights + 2 * f);
        font.height_ = r.u16();
        r.seek(kWidths + 256 * f);
        for (auto& w : font.widths_) {
            w = r.u8();
        }
        std::array<std::uint32_t, 256> offsets{};
        r.seek(kOffsets + 1024 * f);
        for (auto& o : offsets) {
            o = r.u32();
        }
        const std::size_t glyphBytes = entry.size() - kGlyphs;
        for (std::size_t c = 0; c < 256; ++c) {
            if (font.widths_[c] == 0) {
                continue;
            }
            // A glyph runs up to where the next character's starts; the
            // last ones of a font run to where they stop making sense.
            const std::size_t start = offsets[c];
            std::size_t end = c + 1 < 256 ? offsets[c + 1] : glyphBytes;
            if (end < start || end > glyphBytes) {
                end = glyphBytes;
            }
            if (start > glyphBytes) {
                throw FormatError("font glyph outside the entry");
            }
            auto& rows = font.glyphs_[c].rows;
            rows.emplace_back();
            for (std::size_t i = start; i < end; ++i) {
                const std::uint8_t b = entry[kGlyphs + i];
                if (b == kNewRow) {
                    if (rows.size() >= kMaxRows) {
                        break;
                    }
                    rows.emplace_back();
                } else if (b > kNewRow) {
                    rows.back().insert(rows.back().end(), static_cast<std::size_t>(b - kNewRow), 0);
                } else {
                    rows.back().push_back(b);
                }
                if (rows.back().size() > font.widths_[c]) {
                    rows.back().resize(font.widths_[c]);
                }
            }
        }
    }
    return set;
}

int Font::textWidth(std::string_view text, int spacing) const {
    int w = 0;
    for (const char ch : text) {
        w += widths_[static_cast<unsigned char>(ch)] + spacing;
    }
    return w;
}

int Font::draw(Canvas& canvas, int x, int y, std::string_view text, const TextStyle& style,
               const TextClip& clip) const {
    const auto put = [&](int px, int py, std::uint8_t colour) {
        if (clip.contains(px, py) && px >= 0 && py >= 0 && px < canvas.width() && py < canvas.height()) {
            canvas.set(px, py, colour);
        }
    };
    const auto each = [&](auto&& fn) {
        int cx = x;
        for (const char ch : text) {
            const auto c = static_cast<unsigned char>(ch);
            const auto& rows = glyphs_[c].rows;
            for (std::size_t row = 0; row < rows.size(); ++row) {
                for (std::size_t col = 0; col < rows[row].size(); ++col) {
                    if (const auto shade = rows[row][col]; shade != 0) {
                        fn(cx + static_cast<int>(col), y + static_cast<int>(row), shade);
                    }
                }
            }
            cx += widths_[c] + style.spacing;
        }
        return cx;
    };
    if (style.shadow) {
        each([&](int px, int py, std::uint8_t) {
            for (const auto& o : style.shadowAt) {
                put(px + o.dx, py + o.dy, *style.shadow);
            }
        });
    }
    return each([&](int px, int py, std::uint8_t shade) {
        if (shade < style.shades.size()) {
            put(px, py, style.shades[shade]);
        }
    });
}

} // namespace hegemon
