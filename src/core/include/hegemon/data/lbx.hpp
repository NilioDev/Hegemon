// SPDX-License-Identifier: GPL-3.0-or-later
//
// LBX archives: the containers that hold nearly all of the original game's
// pictures, sounds, music, texts and tables. See docs/formats/lbx.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace hegemon {

class LbxArchive {
public:
    // The header is always this long; the entries start after it.
    static constexpr std::size_t kHeaderSize = 2048;
    // As many offsets as fit in the header, less the end offset.
    static constexpr std::size_t kMaxEntries = (kHeaderSize - 8) / 4 - 1;

    // Checks the header and every offset against the size of the data and
    // throws FormatError when anything is out of place. The archive keeps
    // its own copy of the bytes.
    static LbxArchive parse(std::vector<std::uint8_t> bytes);
    static LbxArchive load(const std::filesystem::path& path);

    // True when the bytes start like an LBX archive. Some files with the
    // .LBX extension are videos instead (see docs/formats/lbx.md).
    static bool looksLikeLbx(std::span<const std::uint8_t> bytes);

    std::size_t size() const { return offsets_.empty() ? 0 : offsets_.size() - 1; }
    std::uint16_t type() const { return type_; }

    // The bytes of entry `index`; empty for an empty entry. Throws
    // FormatError for an index past the end.
    std::span<const std::uint8_t> entry(std::size_t index) const;

private:
    std::vector<std::uint8_t> bytes_;
    std::vector<std::uint32_t> offsets_;
    std::uint16_t type_ = 0;
};

} // namespace hegemon
