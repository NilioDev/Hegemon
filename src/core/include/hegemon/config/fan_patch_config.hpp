// SPDX-License-Identifier: GPL-3.0-or-later
//
// The settings of the community fan patch 1.51: ORION2.CFG in the game
// folder, the files it includes and the mods it enables. Hegemon reads them
// as the patched game does, so changes to them work in Hegemon too. See
// docs/formats/config.md.
#pragma once

#include "hegemon/data/reader.hpp"

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

// A settings file that can't be read or makes no sense. The message names
// the file and line.
class ConfigError : public FormatError {
public:
    using FormatError::FormatError;
};

struct ConfigValue {
    enum class Kind { Word, Number, Text };
    Kind kind = Kind::Word;
    std::string text;        // as written, without the quotes of a text
    std::int64_t number = 0; // for Kind::Number
};

// One `name ... = value ...;` or `command argument ...;` of a settings file.
struct ConfigStatement {
    std::vector<std::string> keys; // in lower case; for a command, its name
    bool assignment = false;
    std::vector<ConfigValue> values;
    int line = 0;
};

// Splits a settings file into its statements. Throws ConfigError, naming
// `file`, when a text or a statement isn't closed.
std::vector<ConfigStatement> parseConfig(std::string_view text, std::string_view file);

// A setting as it ended up in the loaded configuration.
struct ConfigEntry {
    std::vector<std::string> keys;
    std::vector<ConfigValue> values;
    // The folder of the file it is written in, as real names from the game
    // folder down; relative paths in its values start there.
    std::vector<std::string> folder;
    std::string file;
    int line = 0;
};

struct ConfigMod {
    std::string id;
    std::string name;
    std::string modClass;
    std::int64_t order = 0;
    std::string file;
};

class FanPatchConfig {
public:
    static constexpr std::string_view kMainFile = "ORION2.CFG";

    // Reads ORION2.CFG from the game folder, the files it includes and the
    // mods enabled along the way. `language` replaces $LANG$ in paths.
    // Throws ConfigError when ORION2.CFG is missing or a file is damaged.
    static FanPatchConfig load(const std::filesystem::path& gameFolder, std::string language = "EN");

    // The last setting with exactly these names (in lower case), or nullptr.
    const ConfigEntry* last(std::initializer_list<std::string_view> keys) const;
    // Every setting whose names start with these, in the order they were read.
    std::vector<const ConfigEntry*> startingWith(std::initializer_list<std::string_view> keys) const;
    const std::vector<ConfigEntry>& entries() const { return entries_; }

    // Where a path written in `entry` leads. A path that starts with `?` is
    // optional: nullopt when the file isn't there. Throws ConfigError when a
    // required file is missing or the path leaves the game folder.
    std::optional<std::filesystem::path> resolve(const ConfigEntry& entry, std::string_view path) const;

    // The file that replaces the LBX archive `name` (such as "COMBAT.LBX"),
    // if the settings name one that exists.
    std::optional<std::filesystem::path> lbxFile(std::string_view name) const;

    // The mods that were loaded, in the order they were loaded.
    const std::vector<ConfigMod>& mods() const { return mods_; }

private:
    std::filesystem::path root_;
    std::string language_;
    std::vector<ConfigEntry> entries_;
    std::vector<ConfigMod> mods_;

    friend class ConfigLoader;
};

} // namespace hegemon
