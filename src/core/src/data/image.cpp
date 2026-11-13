// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/image.hpp"

#include "hegemon/data/reader.hpp"

#include <stdexcept>

namespace hegemon {

namespace {

constexpr std::uint16_t kFrameStart = 1;
constexpr std::uint16_t kFrameEnd = 1000;
constexpr int kMaxSide = 4096;

std::uint8_t widen(std::uint8_t sixBits) {
    const unsigned v = sixBits & 0x3Fu;
    return static_cast<std::uint8_t>((v << 2) | (v >> 4));
}

// Walks one frame. Calls `run(x, y, pixels)` for every run of pixels; the
// run is checked to lie inside the picture before the call.
template <typename Run>
void walkFrame(std::span<const std::uint8_t> frame, int width, int height, Run&& run) {
    Reader r(frame);
    if (r.u16() != kFrameStart) {
        throw FormatError("image frame does not start with its marker");
    }
    long y = r.u16();
    long x = 0;
    for (;;) {
        const std::uint16_t count = r.u16();
        const std::uint16_t step = r.u16();
        if (count == 0) {
            if (step == kFrameEnd) {
                return;
            }
            y += step;
            x = 0;
            if (y > kMaxSide) {
                throw FormatError("image frame runs past its last line");
            }
            continue;
        }
        x += step;
        if (y >= height || x + count > width) {
            throw FormatError("image frame draws outside the picture");
        }
        const auto pixels = r.bytes(count);
        if (count % 2 != 0) {
            r.skip(1);
        }
        run(static_cast<int>(x), static_cast<int>(y), pixels);
        x += count;
    }
}

} // namespace

void Palette::set(std::size_t first, std::span<const std::uint8_t> entries) {
    const std::size_t count = entries.size() / 4;
    if (first > colors.size() || count > colors.size() - first) {
        throw FormatError("palette runs past colour 255");
    }
    for (std::size_t i = 0; i < count; ++i) {
        colors[first + i] = {widen(entries[4 * i + 1]), widen(entries[4 * i + 2]), widen(entries[4 * i + 3])};
    }
}

Canvas::Canvas(int width, int height, std::uint8_t fill)
    : width_(width), height_(height),
      pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill) {}

void toRgba(const Canvas& canvas, const Palette& palette, std::span<std::uint8_t> out) {
    const auto pixels = canvas.pixels();
    if (out.size() < pixels.size() * 4) {
        throw std::length_error("RGBA buffer too small");
    }
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        const Color c = palette.colors[pixels[i]];
        out[4 * i] = c.r;
        out[4 * i + 1] = c.g;
        out[4 * i + 2] = c.b;
        out[4 * i + 3] = 255;
    }
}

Image Image::parse(std::span<const std::uint8_t> entry) {
    Image image;
    Reader r(entry);
    image.width_ = r.u16();
    image.height_ = r.u16();
    const auto reserved = r.u16();
    const std::size_t frameCount = r.u16();
    image.loopFrame_ = r.u16();
    image.flags_ = r.u16();
    if (image.width_ == 0 || image.height_ == 0 || image.width_ > kMaxSide || image.height_ > kMaxSide ||
        reserved != 0 || frameCount == 0) {
        throw FormatError("not a picture");
    }
    std::vector<std::uint32_t> offsets;
    offsets.reserve(frameCount + 1);
    for (std::size_t i = 0; i <= frameCount; ++i) {
        offsets.push_back(r.u32());
    }
    if ((image.flags_ & kFlagPalette) != 0) {
        const std::size_t first = r.u16();
        const std::size_t count = r.u16();
        if (first > 256 || count > 256 - first) {
            throw FormatError("palette runs past colour 255");
        }
        Palette scratch;
        scratch.set(first, r.bytes(4 * count));
        PaletteChange change;
        change.first = first;
        change.colors.assign(scratch.colors.begin() + static_cast<std::ptrdiff_t>(first),
                             scratch.colors.begin() + static_cast<std::ptrdiff_t>(first + count));
        image.palette_ = std::move(change);
    }
    std::size_t previous = r.pos();
    for (const auto offset : offsets) {
        if (offset < previous || offset > entry.size()) {
            throw FormatError("image frame offset out of place");
        }
        previous = offset;
    }
    for (std::size_t i = 0; i < frameCount; ++i) {
        const auto frame = entry.subspan(offsets[i], offsets[i + 1] - offsets[i]);
        if ((image.flags_ & kFlagRaw) != 0) {
            if (frame.size() < static_cast<std::size_t>(image.width_) * static_cast<std::size_t>(image.height_)) {
                throw FormatError("image frame shorter than its pixels");
            }
        } else {
            walkFrame(frame, image.width_, image.height_, [](int, int, std::span<const std::uint8_t>) {});
        }
        image.frames_.emplace_back(frame.begin(), frame.end());
    }
    return image;
}

void Image::applyPalette(Palette& palette) const {
    if (!palette_) {
        return;
    }
    for (std::size_t i = 0; i < palette_->colors.size(); ++i) {
        palette.colors[palette_->first + i] = palette_->colors[i];
    }
}

void Image::draw(std::size_t index, Canvas& canvas, int x, int y) const {
    static const ColourMap same = [] {
        ColourMap m{};
        for (std::size_t i = 0; i < m.size(); ++i) {
            m[i] = static_cast<std::uint8_t>(i);
        }
        return m;
    }();
    draw(index, canvas, x, y, same);
}

void Image::draw(std::size_t index, Canvas& canvas, int x, int y, const ColourMap& colours) const {
    if (index >= frames_.size()) {
        throw FormatError("image frame index out of range");
    }
    const auto put = [&](int px, int py, std::span<const std::uint8_t> pixels) {
        const int cy = y + py;
        if (cy < 0 || cy >= canvas.height()) {
            return;
        }
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            const int cx = x + px + static_cast<int>(i);
            if (cx >= 0 && cx < canvas.width()) {
                canvas.set(cx, cy, colours[pixels[i]]);
            }
        }
    };
    const std::span<const std::uint8_t> frame(frames_[index]);
    if ((flags_ & kFlagRaw) != 0) {
        const auto w = static_cast<std::size_t>(width_);
        for (int row = 0; row < height_; ++row) {
            put(0, row, frame.subspan(static_cast<std::size_t>(row) * w, w));
        }
        return;
    }
    walkFrame(frame, width_, height_, put);
}

ColourMap nearestColours(const Palette& from, const Palette& to) {
    // Back to the original's six bits; widen() keeps them in the top bits.
    const auto six = [](std::uint8_t v) { return v >> 2; };
    ColourMap out{};
    for (std::size_t i = 0; i < from.colors.size(); ++i) {
        const Color& c = from.colors[i];
        int best = -1;
        for (std::size_t j = 1; j < to.colors.size(); ++j) {
            const Color& d = to.colors[j];
            const int dr = six(c.r) - six(d.r);
            const int dg = six(c.g) - six(d.g);
            const int db = six(c.b) - six(d.b);
            const int dist = dr * dr + dg * dg + db * db;
            if (best < 0 || dist < best) {
                best = dist;
                out[i] = static_cast<std::uint8_t>(j);
            }
        }
    }
    return out;
}

} // namespace hegemon
