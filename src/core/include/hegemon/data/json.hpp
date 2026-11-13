// SPDX-License-Identifier: GPL-3.0-or-later
//
// A small reader for JSON, enough for the fan patch's list of settings
// (150\docs\config.json). It refuses anything malformed, nested too deep or
// too big, so a damaged file can't make Hegemon misbehave.
#pragma once

#include "hegemon/data/reader.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace hegemon {

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue, std::less<>>;

    // Throws FormatError, with the byte where it went wrong, when `text`
    // isn't one JSON value.
    static JsonValue parse(std::string_view text);

    bool isNull() const { return std::holds_alternative<std::monostate>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumber() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isArray() const { return std::holds_alternative<std::shared_ptr<Array>>(value_); }
    bool isObject() const { return std::holds_alternative<std::shared_ptr<Object>>(value_); }

    // These throw FormatError when the value is of another kind.
    bool boolean() const;
    double number() const;
    // A number that is whole and fits in 64 bits.
    std::int64_t integer() const;
    const std::string& string() const;
    const Array& array() const;
    const Object& object() const;

    // The member `key` of an object, or nullptr when there is none or this
    // isn't an object.
    const JsonValue* find(std::string_view key) const;

private:
    friend class JsonParser;
    std::variant<std::monostate, bool, double, std::string, std::shared_ptr<Array>, std::shared_ptr<Object>> value_;
};

} // namespace hegemon
