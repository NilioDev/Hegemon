// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/game_files.hpp"

#include <cctype>
#include <system_error>

namespace hegemon {

namespace {

bool equalsNoCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<std::filesystem::path> findFileNoCase(const std::filesystem::path& folder,
                                                    std::string_view name) {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (entry.is_regular_file(ec) && equalsNoCase(entry.path().filename().string(), name)) {
            return entry.path();
        }
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> findPathNoCase(std::filesystem::path folder, std::string_view relative) {
    while (!relative.empty()) {
        const auto slash = relative.find('/');
        const auto part = relative.substr(0, slash);
        const bool last = slash == std::string_view::npos;
        std::optional<std::filesystem::path> found;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
            if (entry.is_symlink(ec) || !equalsNoCase(entry.path().filename().string(), part)) {
                continue;
            }
            if (last ? entry.is_regular_file(ec) : entry.is_directory(ec)) {
                found = entry.path();
                break;
            }
        }
        if (!found) {
            return std::nullopt;
        }
        folder = *found;
        relative = last ? std::string_view{} : relative.substr(slash + 1);
    }
    return folder;
}


bool isGameFolder(const std::filesystem::path& folder) {
    for (const auto name : kMarkerFiles) {
        if (!findFileNoCase(folder, name)) {
            return false;
        }
    }
    return true;
}

} // namespace hegemon
