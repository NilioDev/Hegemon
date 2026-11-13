// SPDX-License-Identifier: GPL-3.0-or-later
//
// The player's game files, opened as they are needed.
#pragma once

#include "hegemon/config/fan_patch_config.hpp"
#include "hegemon/config/parameters.hpp"
#include "hegemon/data/lbx.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace hegemon {

class GameData {
public:
    // Reads the fan patch's settings when the folder has them. Throws
    // ConfigError when they are damaged.
    explicit GameData(std::filesystem::path folder);

    const std::filesystem::path& folder() const { return folder_; }

    // The fan patch's settings, or nullptr when the fan patch isn't
    // installed in the folder.
    const FanPatchConfig* config() const { return config_ ? &*config_ : nullptr; }

    // The fan patch's rule settings: the defaults from its list
    // (150\docs\config.json) with the settings files applied. Read once.
    // Throws FormatError when the fan patch or its list is missing or
    // damaged, ConfigError when a setting isn't allowed.
    const Parameters& parameters();

    // True when the folder has the file, in any case.
    bool hasFile(std::string_view name) const;

    // Opens an LBX archive by its name once and keeps it: the file the fan
    // patch's settings name for it, if any, or else the one in the folder.
    // Throws FormatError when the file is missing or damaged.
    const LbxArchive& archive(std::string_view name);

private:
    std::filesystem::path folder_;
    std::optional<FanPatchConfig> config_;
    std::optional<Parameters> parameters_;
    std::map<std::string, LbxArchive, std::less<>> archives_;
};

} // namespace hegemon
