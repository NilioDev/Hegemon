// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/config/parameters.hpp"

#include "hegemon/data/json.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace hegemon {

namespace {

std::string lower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

ParameterType readType(const JsonValue& v) {
    ParameterType t;
    const auto& type = v.find("type") ? v.find("type")->string() : std::string("integer");
    if (type == "integer") {
        t.kind = ParameterType::Kind::Integer;
        if (const auto* m = v.find("min")) {
            t.min = m->integer();
        }
        if (const auto* m = v.find("max")) {
            t.max = m->integer();
        }
    } else if (type == "enum") {
        t.kind = ParameterType::Kind::Enum;
        if (const auto* w = v.find("values")) {
            for (const auto& word : w->array()) {
                t.words.push_back(lower(word.string()));
            }
        }
    } else if (type == "string") {
        t.kind = ParameterType::Kind::Text;
    } else if (type == "mask") {
        t.kind = ParameterType::Kind::Mask;
    } else if (type == "ratio") {
        t.kind = ParameterType::Kind::Ratio;
    } else {
        throw FormatError("settings list: unknown type " + type);
    }
    return t;
}

ConfigValue fromJson(const JsonValue& v) {
    ConfigValue c;
    if (v.isNumber()) {
        c.kind = ConfigValue::Kind::Number;
        c.number = v.integer();
        c.text = std::to_string(c.number);
    } else if (v.isString()) {
        c.kind = ConfigValue::Kind::Word;
        c.text = v.string();
    } else if (v.isNull()) {
        c.kind = ConfigValue::Kind::Number; // not set
    } else {
        throw FormatError("settings list: a default is neither a number nor a text");
    }
    return c;
}

// The defaults of a table, nested one list per dimension, flattened.
void flatten(const JsonValue& v, std::size_t depth, std::vector<ConfigValue>& out) {
    if (depth == 0 && v.isArray()) {
        for (const auto& item : v.array()) {
            flatten(item, 0, out);
        }
        return;
    }
    if (depth == 0 || !v.isArray()) { // a single value for a whole level
        out.push_back(fromJson(v));
        return;
    }
    for (const auto& item : v.array()) {
        flatten(item, depth - 1, out);
    }
}

[[noreturn]] void refuse(const ConfigEntry& e, const std::string& what) {
    throw ConfigError(e.file + " line " + std::to_string(e.line) + ": " + what);
}

std::optional<std::size_t> indexOf(const std::vector<std::string>& names, std::string_view name) {
    const auto it = std::find(names.begin(), names.end(), lower(name));
    if (it == names.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - names.begin());
}

} // namespace

std::size_t ParameterDefinition::rowCount() const {
    std::size_t n = 1;
    for (const auto& dim : rowNames) {
        n *= dim.size();
    }
    return n;
}

Parameters Parameters::defaults(std::string_view list) {
    const JsonValue doc = JsonValue::parse(list);
    Parameters p;
    if (const auto* v = doc.find("patch_version")) {
        p.version_ = v->isString() ? v->string() : std::to_string(v->number());
    }
    const auto* params = doc.find("parameters");
    if (params == nullptr) {
        throw FormatError("settings list: no parameters");
    }
    for (const auto& item : params->array()) {
        Setting s;
        auto& d = s.definition;
        d.name = lower(item.find("name") ? item.find("name")->string() : throw FormatError("settings list: no name"));
        const auto* type = item.find("type");
        d.table = type != nullptr && type->string() == "table";
        const JsonValue* def = item.find("default");
        if (d.table) {
            const auto* columns = item.find("columns");
            if (columns == nullptr || columns->array().empty()) {
                throw FormatError("settings list: table " + d.name + " without columns");
            }
            for (const auto& c : columns->array()) {
                d.columns.push_back(lower(c.find("name") ? c.find("name")->string() : ""));
                d.types.push_back(readType(c));
            }
            if (const auto* rows = item.find("rows")) {
                for (const auto& dim : rows->array()) {
                    std::vector<std::string> names;
                    for (const auto& n : dim.array()) {
                        names.push_back(lower(n.string()));
                    }
                    d.rowNames.push_back(std::move(names));
                }
            }
            if (def != nullptr) {
                // One list per dimension, then the columns, unless the
                // table has a single column.
                flatten(*def, d.rowNames.size() + (d.columns.size() > 1 ? 1 : 0), s.cells);
            }
        } else {
            d.types.push_back(readType(item));
            if (def != nullptr) {
                s.cells.push_back(fromJson(*def));
            }
        }
        const std::size_t cells = d.rowCount() * d.columnCount();
        if (s.cells.size() != cells) {
            // A default given once stands for every cell.
            if (s.cells.size() == 1) {
                s.cells.assign(cells, s.cells.front());
            } else if (s.cells.empty()) {
                s.cells.assign(cells, ConfigValue{ConfigValue::Kind::Number, "0", 0});
            } else {
                throw FormatError("settings list: defaults of " + d.name + " don't fit the table");
            }
        }
        const std::string name = d.name;
        p.settings_.insert_or_assign(name, std::move(s));
    }
    return p;
}

Parameters Parameters::load(std::string_view list, const FanPatchConfig& config) {
    Parameters p = defaults(list);
    for (const auto& entry : config.entries()) {
        p.apply(entry);
    }
    return p;
}

void Parameters::apply(const ConfigEntry& e) {
    if (e.keys.empty()) {
        return;
    }
    const auto it = settings_.find(e.keys.front());
    if (it == settings_.end()) {
        return; // the loader's own settings, and any the list doesn't know
    }
    Setting& s = it->second;
    const auto& d = s.definition;
    const std::size_t dims = d.rowNames.size();
    if (e.keys.size() < 1 + dims) {
        refuse(e, d.name + " needs " + std::to_string(dims) + " row names");
    }
    std::size_t row = 0;
    for (std::size_t i = 0; i < dims; ++i) {
        const auto at = indexOf(d.rowNames[i], e.keys[1 + i]);
        if (!at) {
            refuse(e, d.name + " has no row " + e.keys[1 + i]);
        }
        row = row * d.rowNames[i].size() + *at;
    }
    std::size_t first = 0;
    std::size_t count = d.columnCount();
    if (e.keys.size() == 2 + dims) {
        if (!d.table) {
            refuse(e, d.name + " has no columns");
        }
        const auto at = indexOf(d.columns, e.keys[1 + dims]);
        if (!at) {
            refuse(e, d.name + " has no column " + e.keys[1 + dims]);
        }
        first = *at;
        count = 1;
    } else if (e.keys.size() > 2 + dims) {
        refuse(e, "too many names for " + d.name);
    }
    if (e.values.size() != count) {
        refuse(e, d.name + " takes " + std::to_string(count) + " values");
    }
    for (std::size_t i = 0; i < count; ++i) {
        const ParameterType& t = d.types[first + i];
        ConfigValue v = e.values[i];
        if (t.kind == ParameterType::Kind::Integer) {
            if (v.kind == ConfigValue::Kind::Word) {
                // Letters before a number are a label, as in "sd3".
                std::size_t k = 0;
                while (k < v.text.size() && std::isalpha(static_cast<unsigned char>(v.text[k]))) {
                    ++k;
                }
                const std::string rest = v.text.substr(k);
                if (k == 0 || rest.empty() || rest.find_first_not_of("-0123456789") != std::string::npos) {
                    refuse(e, d.name + " takes a number, not " + v.text);
                }
                v.kind = ConfigValue::Kind::Number;
                v.number = std::stoll(rest);
            } else if (v.kind != ConfigValue::Kind::Number) {
                refuse(e, d.name + " takes a number");
            }
            if ((t.min && v.number < *t.min) || (t.max && v.number > *t.max)) {
                refuse(e, d.name + " out of range: " + std::to_string(v.number));
            }
        } else if (t.kind == ParameterType::Kind::Enum) {
            if (v.kind == ConfigValue::Kind::Number) {
                if (v.number < 0 || static_cast<std::size_t>(v.number) >= t.words.size()) {
                    refuse(e, d.name + " out of range: " + std::to_string(v.number));
                }
            } else if (!indexOf(t.words, v.text)) {
                refuse(e, d.name + " can't be " + v.text);
            }
        }
        s.cells[row * d.columnCount() + first + i] = std::move(v);
    }
}

const Parameters::Setting& Parameters::setting(std::string_view name) const {
    const auto it = settings_.find(name);
    if (it == settings_.end()) {
        throw std::out_of_range("unknown setting " + std::string(name));
    }
    return it->second;
}

const ParameterDefinition* Parameters::definition(std::string_view name) const {
    const auto it = settings_.find(name);
    return it == settings_.end() ? nullptr : &it->second.definition;
}

std::int64_t Parameters::asInteger(const Setting& s, std::size_t cell) {
    const ConfigValue& v = s.cells.at(cell);
    if (v.kind == ConfigValue::Kind::Number) {
        return v.number;
    }
    const auto& t = s.definition.types[cell % s.definition.columnCount()];
    if (t.kind == ParameterType::Kind::Enum) {
        if (const auto at = indexOf(t.words, v.text)) {
            return static_cast<std::int64_t>(*at);
        }
    }
    throw ConfigError(s.definition.name + " isn't a number: " + v.text);
}

std::int64_t Parameters::integer(std::string_view name) const { return asInteger(setting(name), 0); }

std::int64_t Parameters::integer(std::string_view name, std::size_t row, std::size_t column) const {
    const Setting& s = setting(name);
    if (row >= s.definition.rowCount() || column >= s.definition.columnCount()) {
        throw std::out_of_range("no such cell of " + std::string(name));
    }
    return asInteger(s, row * s.definition.columnCount() + column);
}

std::int64_t Parameters::integer(std::string_view name, std::initializer_list<std::string_view> row,
                                 std::string_view column) const {
    const Setting& s = setting(name);
    const auto& d = s.definition;
    if (row.size() != d.rowNames.size()) {
        throw std::out_of_range("wrong number of row names for " + std::string(name));
    }
    std::size_t r = 0;
    std::size_t i = 0;
    for (const auto& n : row) {
        const auto at = indexOf(d.rowNames[i], n);
        if (!at) {
            throw std::out_of_range("no row " + std::string(n) + " in " + std::string(name));
        }
        r = r * d.rowNames[i].size() + *at;
        ++i;
    }
    const auto c = indexOf(d.columns, column);
    if (!c) {
        throw std::out_of_range("no column " + std::string(column) + " in " + std::string(name));
    }
    return asInteger(s, r * d.columnCount() + *c);
}

const ConfigValue& Parameters::value(std::string_view name, std::size_t row, std::size_t column) const {
    const Setting& s = setting(name);
    if (row >= s.definition.rowCount() || column >= s.definition.columnCount()) {
        throw std::out_of_range("no such cell of " + std::string(name));
    }
    return s.cells[row * s.definition.columnCount() + column];
}

} // namespace hegemon
