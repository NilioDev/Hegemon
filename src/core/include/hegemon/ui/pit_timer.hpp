// SPDX-License-Identifier: GPL-3.0-or-later
//
// The original counts time in ticks of the PC timer: 1,193,182 Hz divided by
// 65,536, about 18.2 ticks a second.
#pragma once

#include <cstdint>

namespace hegemon {

class PitTimer {
public:
    static constexpr std::uint64_t kHz = 1193182;
    static constexpr std::uint64_t kDivisor = 65536;

    // Lets `ms` milliseconds pass and returns how many whole ticks that
    // completed. Time adds up across calls, so short frames lose nothing.
    std::uint64_t advance(std::uint64_t ms) {
        const std::uint64_t before = ticks();
        micros_ += ms * 1000;
        return ticks() - before;
    }

private:
    std::uint64_t ticks() const { return micros_ * kHz / (kDivisor * 1000000); }
    std::uint64_t micros_ = 0;
};

} // namespace hegemon
