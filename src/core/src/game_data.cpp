// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/game_data.hpp"

#include "hegemon/data/reader.hpp"
#include "hegemon/game_files.hpp"

namespace hegemon {

bool GameData::hasFile(std::string_view name) const {
    return findFileNoCase(folder_, name).has_value();
}

const LbxArchive& GameData::archive(std::string_view name) {
    if (const auto it = archives_.find(name); it != archives_.end()) {
        return it->second;
    }
    const auto path = findFileNoCase(folder_, name);
    if (!path) {
        throw FormatError("game file missing: " + std::string(name));
    }
    return archives_.emplace(std::string(name), LbxArchive::load(*path)).first->second;
}

} // namespace hegemon
