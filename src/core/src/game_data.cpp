// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/game_data.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"

#include <fstream>
#include <sstream>

namespace hegemon {


GameData::GameData(std::filesystem::path folder) : folder_(std::move(folder)) {
    if (findFileNoCase(folder_, FanPatchConfig::kMainFile)) {
        config_ = FanPatchConfig::load(folder_);
    }
}

bool GameData::hasFile(std::string_view name) const {
    return findFileNoCase(folder_, name).has_value();
}

const Parameters& GameData::parameters() {
    if (!parameters_) {
        if (!config_) {
            throw FormatError("the fan patch is not installed");
        }
        const auto path = findPathNoCase(folder_, Parameters::kListFile);
        if (!path) {
            throw FormatError("game file missing: " + std::string(Parameters::kListFile));
        }
        std::ifstream in(*path, std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        if (!in && !in.eof()) {
            throw FormatError("could not read " + path->string());
        }
        parameters_ = Parameters::load(text.str(), *config_);
    }
    return *parameters_;
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
