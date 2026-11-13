// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/random.hpp"

#include <stdexcept>

namespace hegemon {

namespace {

std::uint64_t splitmix(std::uint64_t& x) {
    std::uint64_t z = (x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

} // namespace

Random::Random(std::uint64_t seed) {
    for (auto& s : s_) {
        s = splitmix(seed);
    }
}

std::uint64_t Random::next() {
    const std::uint64_t result = rotl(s_[1] * 5, 7) * 9;
    const std::uint64_t t = s_[1] << 17;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = rotl(s_[3], 45);
    return result;
}

std::uint32_t Random::below(std::uint32_t n) {
    if (n == 0) {
        throw std::invalid_argument("Random::below(0)");
    }
    // Drop the few numbers that would make the low ones more likely.
    const std::uint64_t limit = UINT64_MAX - UINT64_MAX % n;
    std::uint64_t r = 0;
    do {
        r = next();
    } while (r >= limit);
    return static_cast<std::uint32_t>(r % n);
}

int Random::between(int lo, int hi) {
    if (lo > hi) {
        throw std::invalid_argument("Random::between: lo above hi");
    }
    const auto span = static_cast<std::uint32_t>(static_cast<std::int64_t>(hi) - lo + 1);
    return static_cast<int>(lo + static_cast<std::int64_t>(below(span)));
}

bool Random::chance(int percent) { return static_cast<int>(below(100)) < percent; }

std::optional<std::size_t> Random::pick(std::span<const std::int64_t> weights) {
    std::int64_t total = 0;
    for (const auto w : weights) {
        if (w > 0) {
            total += w;
        }
    }
    if (total <= 0) {
        return std::nullopt;
    }
    if (total > static_cast<std::int64_t>(UINT32_MAX)) {
        throw std::invalid_argument("Random::pick: weights too big");
    }
    auto r = static_cast<std::int64_t>(below(static_cast<std::uint32_t>(total)));
    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] > 0) {
            if (r < weights[i]) {
                return i;
            }
            r -= weights[i];
        }
    }
    return std::nullopt; // not reached
}

} // namespace hegemon
