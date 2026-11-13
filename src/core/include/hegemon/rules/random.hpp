// SPDX-License-Identifier: GPL-3.0-or-later
//
// Hegemon's own random numbers: the same seed gives the same numbers on
// every system and compiler, so a galaxy or a game can be made again from
// its seed (the standard library's distributions differ between systems).
#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace hegemon {

class Random {
public:
    explicit Random(std::uint64_t seed);

    // 64 random bits (xoshiro256**).
    std::uint64_t next();
    // A number from 0 to n - 1; n must be at least 1.
    std::uint32_t below(std::uint32_t n);
    // A number from `lo` to `hi`, both included; lo must not be above hi.
    int between(int lo, int hi);
    // True with a chance of `percent` in a hundred.
    bool chance(int percent);
    // An index chosen with the given weights; none when they add up to 0.
    // Negative weights count as 0.
    std::optional<std::size_t> pick(std::span<const std::int64_t> weights);

private:
    std::uint64_t s_[4];
};

} // namespace hegemon
