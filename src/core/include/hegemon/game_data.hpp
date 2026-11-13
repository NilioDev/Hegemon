// SPDX-License-Identifier: GPL-3.0-or-later
//
// The player's game files, opened as they are needed.
#pragma once

#include "hegemon/data/lbx.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace hegemon {

class GameData {
public:
    explicit GameData(std::filesystem::path folder) : folder_(std::move(folder)) {}

    const std::filesystem::path& folder() const { return folder_; }

    // True when the folder has the file, in any case.
    bool hasFile(std::string_view name) const;

    // Opens an LBX archive by its name once and keeps it. Throws FormatError
    // when the file is missing or damaged.
    const LbxArchive& archive(std::string_view name);

private:
    std::filesystem::path folder_;
    std::map<std::string, LbxArchive, std::less<>> archives_;
};

} // namespace hegemon
