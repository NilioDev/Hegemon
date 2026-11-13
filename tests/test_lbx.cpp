// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

#include "hegemon/data/lbx.hpp"
#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

using hegemon::FormatError;
using hegemon::LbxArchive;

namespace {

void put16(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    out[at] = static_cast<std::uint8_t>(v);
    out[at + 1] = static_cast<std::uint8_t>(v >> 8);
}

void put32(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    put16(out, at, v & 0xFFFF);
    put16(out, at + 2, v >> 16);
}

// An archive made by hand: entries of the given sizes, filled with their index.
std::vector<std::uint8_t> makeArchive(const std::vector<std::size_t>& sizes) {
    std::vector<std::uint8_t> out(LbxArchive::kHeaderSize, 0);
    put16(out, 0, static_cast<std::uint32_t>(sizes.size()));
    put32(out, 2, 0xFEAD);
    put16(out, 6, 5);
    std::uint32_t offset = LbxArchive::kHeaderSize;
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        put32(out, 8 + 4 * i, offset);
        out.insert(out.end(), sizes[i], static_cast<std::uint8_t>(i));
        offset += static_cast<std::uint32_t>(sizes[i]);
    }
    put32(out, 8 + 4 * sizes.size(), offset);
    return out;
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

} // namespace

HGM_TEST("LBX: entries come back with their own bytes") {
    const auto archive = LbxArchive::parse(makeArchive({3, 0, 5}));
    HGM_CHECK(archive.size() == 3);
    HGM_CHECK(archive.type() == 5);
    HGM_CHECK(archive.entry(0).size() == 3);
    HGM_CHECK(archive.entry(0)[2] == 0);
    HGM_CHECK(archive.entry(1).empty());
    HGM_CHECK(archive.entry(2).size() == 5);
    HGM_CHECK(archive.entry(2)[4] == 2);
    HGM_CHECK(throwsFormatError([&] { (void)archive.entry(3); }));
}

HGM_TEST("LBX: damaged headers are refused") {
    HGM_CHECK(throwsFormatError([] { (void)LbxArchive::parse(std::vector<std::uint8_t>(100, 0)); }));

    auto badSignature = makeArchive({4});
    badSignature[2] = 0;
    HGM_CHECK(!LbxArchive::looksLikeLbx(badSignature));
    HGM_CHECK(throwsFormatError([&] { (void)LbxArchive::parse(badSignature); }));

    auto pastEnd = makeArchive({4});
    put32(pastEnd, 12, static_cast<std::uint32_t>(pastEnd.size() + 1));
    HGM_CHECK(throwsFormatError([&] { (void)LbxArchive::parse(pastEnd); }));

    auto backwards = makeArchive({4, 4});
    put32(backwards, 12, LbxArchive::kHeaderSize - 1);
    HGM_CHECK(throwsFormatError([&] { (void)LbxArchive::parse(backwards); }));

    auto tooMany = makeArchive({4});
    put16(tooMany, 0, LbxArchive::kMaxEntries + 1);
    HGM_CHECK(throwsFormatError([&] { (void)LbxArchive::parse(tooMany); }));
}

// With the player's files in gamedata/: every archive of the original opens,
// and the files that are videos are recognised as not being archives.
HGM_TEST("LBX: every archive of the original opens") {
    const std::filesystem::path folder = HEGEMON_GAMEDATA_DIR;
    if (!hegemon::isGameFolder(folder)) {
        HGM_SKIP("original game files not in gamedata/");
    }
    std::size_t archives = 0;
    std::size_t entries = 0;
    std::size_t videos = 0;
    for (const auto& file : std::filesystem::directory_iterator(folder)) {
        auto ext = file.path().extension().string();
        for (auto& c : ext) {
            c = static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
        }
        if (ext != ".LBX") {
            continue;
        }
        std::ifstream in(file.path(), std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!LbxArchive::looksLikeLbx(bytes)) {
            // The opening and the endings are Smacker videos.
            HGM_CHECK(bytes.size() > 4 && bytes[0] == 'S' && bytes[1] == 'M' && bytes[2] == 'K');
            ++videos;
            continue;
        }
        const auto archive = LbxArchive::parse(std::move(bytes));
        ++archives;
        entries += archive.size();
        HGM_CHECK(archive.size() > 0);
    }
    HGM_CHECK(archives > 300);
    HGM_CHECK(entries > archives);
    HGM_CHECK(videos > 0);
}
