// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/lbx.hpp"

#include "hegemon/data/reader.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace hegemon {

namespace {

constexpr std::uint32_t kSignature = 0x0000FEADu;

} // namespace

bool LbxArchive::looksLikeLbx(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kHeaderSize) {
        return false;
    }
    Reader r(bytes);
    const auto count = r.u16();
    return r.u32() == kSignature && count <= kMaxEntries;
}

LbxArchive LbxArchive::parse(std::vector<std::uint8_t> bytes) {
    if (bytes.size() < kHeaderSize) {
        throw FormatError("LBX archive shorter than its header");
    }
    LbxArchive archive;
    Reader r(bytes);
    const std::size_t count = r.u16();
    if (r.u32() != kSignature) {
        throw FormatError("not an LBX archive");
    }
    if (count > kMaxEntries) {
        throw FormatError("LBX archive lists more entries than its header holds");
    }
    archive.type_ = r.u16();
    archive.offsets_.reserve(count + 1);
    for (std::size_t i = 0; i <= count; ++i) {
        const auto offset = r.u32();
        const std::uint32_t previous = archive.offsets_.empty() ? kHeaderSize : archive.offsets_.back();
        if (offset < previous || offset > bytes.size()) {
            throw FormatError("LBX entry offset out of place");
        }
        archive.offsets_.push_back(offset);
    }
    archive.bytes_ = std::move(bytes);
    return archive;
}

LbxArchive LbxArchive::load(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw FormatError("cannot open " + path.string());
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parse(std::move(bytes));
}

std::span<const std::uint8_t> LbxArchive::entry(std::size_t index) const {
    if (index >= size()) {
        throw FormatError("LBX entry index out of range");
    }
    const std::span<const std::uint8_t> all(bytes_);
    return all.subspan(offsets_[index], offsets_[index + 1] - offsets_[index]);
}

} // namespace hegemon
