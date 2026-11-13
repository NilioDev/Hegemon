// SPDX-License-Identifier: GPL-3.0-or-later
//
// The fan patch's rule settings with their values: the defaults from the
// list the fan patch ships (150\docs\config.json), changed by the settings
// files it reads. See docs/formats/parameters.md.
#pragma once

#include "hegemon/config/fan_patch_config.hpp"

#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hegemon {

class JsonValue;

// What one value of a setting may be.
struct ParameterType {
    enum class Kind { Integer, Enum, Text, Mask, Ratio };
    Kind kind = Kind::Integer;
    std::optional<std::int64_t> min, max; // for Integer
    std::vector<std::string> words;       // for Enum, in the order of their numbers
};

// One setting as the fan patch's list describes it: a single value, or a
// table with rows named along zero or more dimensions and named columns.
struct ParameterDefinition {
    std::string name;
    bool table = false;
    std::vector<std::string> columns;               // empty for a single value
    std::vector<ParameterType> types;               // one per column, or one
    std::vector<std::vector<std::string>> rowNames; // one list per dimension

    std::size_t rowCount() const;
    std::size_t columnCount() const { return types.size(); }
};

class Parameters {
public:
    // The fan patch's list, relative to the game folder.
    static constexpr std::string_view kListFile = "150/docs/config.json";

    // Reads the list (the text of config.json) and applies every setting of
    // `config` it knows. Throws FormatError when the list is damaged and
    // ConfigError, naming the file and line, when a setting's value isn't
    // allowed.
    static Parameters load(std::string_view list, const FanPatchConfig& config);
    // Only the defaults, for tests.
    static Parameters defaults(std::string_view list);

    // The version of the fan patch the list is for, such as "1.51".
    const std::string& version() const { return version_; }

    const ParameterDefinition* definition(std::string_view name) const;

    // A setting with a single value, as a number (an Enum gives the number
    // of its word). Throws std::out_of_range for an unknown name and
    // ConfigError when the value isn't a number.
    std::int64_t integer(std::string_view name) const;
    // A cell of a table: the row's name along each dimension (none for a
    // table of one row), and the column's name.
    std::int64_t integer(std::string_view name, std::initializer_list<std::string_view> row,
                         std::string_view column) const;
    // The same by numbers: row `row` counted across all dimensions, the
    // first dimension changing slowest.
    std::int64_t integer(std::string_view name, std::size_t row, std::size_t column) const;
    // The value as written (a word, a text or a number).
    const ConfigValue& value(std::string_view name, std::size_t row = 0, std::size_t column = 0) const;

private:
    struct Setting {
        ParameterDefinition definition;
        std::vector<ConfigValue> cells; // row by row
    };

    void apply(const ConfigEntry& entry);
    const Setting& setting(std::string_view name) const;
    static std::int64_t asInteger(const Setting& s, std::size_t cell);

    std::string version_;
    std::map<std::string, Setting, std::less<>> settings_;
};

} // namespace hegemon
