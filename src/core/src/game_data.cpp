// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/game_data.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"

namespace hegemon {

GameData::GameData(std::filesystem::path folder) : folder_(std::move(folder)) {
    if (findFileNoCase(folder_, FanPatchConfig::kMainFile)) {
        config_ = FanPatchConfig::load(folder_);
    }
}

bool GameData::hasFile(std::string_view name) const {
    return findFileNoCase(folder_, name).has_value();
}

const LbxArchive& GameData::archive(std::string_view name) {
    if (const auto it = archives_.find(name); it != archives_.end()) {
        return it->second;
    }
    auto path = config_ ? config_->lbxFile(name) : std::nullopt;
    if (!path) {
        path = findFileNoCase(folder_, name);
    }
    if (!path) {
        throw FormatError("game file missing: " + std::string(name));
    }
    return archives_.emplace(std::string(name), LbxArchive::load(*path)).first->second;
}

} // namespace hegemon
