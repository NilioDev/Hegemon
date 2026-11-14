// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/rules/tech.hpp"

#include "hegemon/config/parameters.hpp"
#include "hegemon/data/reader.hpp"

#include <stdexcept>
#include <string_view>

namespace hegemon {

namespace {

// Field ids above this are not real (the settings allow 16 bits).
constexpr std::int64_t kMaxId = 4096;

const std::vector<std::string>& rowNames(const Parameters& p, std::string_view name) {
    const ParameterDefinition* d = p.definition(name);
    if (d == nullptr || d->rowNames.size() != 1) {
        throw std::out_of_range("setting " + std::string(name) + " missing or not a table of named rows");
    }
    return d->rowNames[0];
}

std::size_t columnOf(const Parameters& p, std::string_view name, std::string_view column) {
    const ParameterDefinition* d = p.definition(name);
    for (std::size_t i = 0; d != nullptr && i < d->columns.size(); ++i) {
        if (d->columns[i] == column) {
            return i;
        }
    }
    throw std::out_of_range("setting " + std::string(name) + " has no column " + std::string(column));
}

int checkedId(std::int64_t id) {
    if (id < 0 || id > kMaxId) {
        throw FormatError("tech id out of range");
    }
    return static_cast<int>(id);
}

// The field a column of starting_techfield or prewarp_techfield names.
int namedField(const Parameters& p, const std::vector<TechField>& fields, std::string_view setting, std::size_t column) {
    const ConfigValue& v = p.value(setting, 0, column);
    for (const TechField& f : fields) {
        if (f.key == v.text) {
            return f.id;
        }
    }
    // An Enum's number counts from starting_tech, as the field ids do.
    const auto id = p.integer(setting, 0, column);
    if (id < 0 || static_cast<std::size_t>(id) >= fields.size()) {
        throw FormatError(std::string(setting) + " names an unknown field");
    }
    return static_cast<int>(id);
}

} // namespace

bool Knowledge::knowsField(int id) const {
    return id >= 0 && static_cast<std::size_t>(id) < fields.size() && fields[static_cast<std::size_t>(id)];
}

bool Knowledge::knowsTech(int id) const {
    return id >= 0 && static_cast<std::size_t>(id) < techs.size() && techs[static_cast<std::size_t>(id)];
}

TechTree::TechTree(const Parameters& p) {
    const auto& fieldNames = rowNames(p, "tech_field");
    const std::size_t idCol = columnOf(p, "tech_field", "id"), nextCol = columnOf(p, "tech_field", "next_field_id"),
                      costCol = columnOf(p, "tech_field", "cost");
    fields_.resize(1);
    fields_[0].key = "starting_tech";
    struct Row {
        int id, next, cost;
    };
    std::vector<Row> rows;
    for (std::size_t r = 0; r < fieldNames.size(); ++r) {
        const int id = checkedId(p.integer("tech_field", r, idCol));
        const int next = checkedId(p.integer("tech_field", r, nextCol));
        if (id == 0) {
            throw FormatError("tech_field: a field with id 0");
        }
        if (static_cast<std::size_t>(id) >= fields_.size()) {
            fields_.resize(static_cast<std::size_t>(id) + 1);
        }
        TechField& f = fields_[static_cast<std::size_t>(id)];
        f.id = id;
        f.key = fieldNames[r];
        f.next = next;
        f.cost = static_cast<int>(p.integer("tech_field", r, costCol));
    }
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].id = static_cast<int>(i);
        if (fields_[i].next < 0 || static_cast<std::size_t>(fields_[i].next) >= fields_.size()) {
            fields_[i].next = 0;
        }
    }

    const auto& techNames = rowNames(p, "tech");
    const std::size_t techIdCol = columnOf(p, "tech", "id"), techFieldCol = columnOf(p, "tech", "tech_field_id");
    for (std::size_t r = 0; r < techNames.size(); ++r) {
        const int id = checkedId(p.integer("tech", r, techIdCol));
        const auto field = p.integer("tech", r, techFieldCol);
        if (static_cast<std::size_t>(id) >= techFields_.size()) {
            techFields_.resize(static_cast<std::size_t>(id) + 1, -1);
        }
        if (field >= 0 && static_cast<std::size_t>(field) < fields_.size()) {
            techFields_[static_cast<std::size_t>(id)] = static_cast<int>(field);
            fields_[static_cast<std::size_t>(field)].techs.push_back(id);
        }
    }
    techCount_ = techFields_.size();

    // Each category's chain, from its first field along next_field_id.
    for (std::size_t c = 0; c < kTechCategories; ++c) {
        const int start = namedField(p, fields_, "starting_techfield", c);
        first_[c] = start;
        int at = start;
        for (std::size_t steps = 0; at != 0 && steps < fields_.size(); ++steps) {
            TechField& f = fields_[static_cast<std::size_t>(at)];
            if (f.category >= 0) {
                break; // a field that leads to itself, such as Xenon Technology
            }
            f.category = static_cast<int>(c);
            at = f.next;
        }
    }
    const ParameterDefinition* pw = p.definition("prewarp_techfield");
    for (std::size_t c = 0; pw != nullptr && c < pw->columnCount(); ++c) {
        prewarp_.push_back(namedField(p, fields_, "prewarp_techfield", c));
    }
}

const TechField& TechTree::field(int id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= fields_.size()) {
        throw std::out_of_range("no such tech field");
    }
    return fields_[static_cast<std::size_t>(id)];
}

int TechTree::techField(int tech) const {
    if (tech < 0 || static_cast<std::size_t>(tech) >= techFields_.size()) {
        return -1;
    }
    return techFields_[static_cast<std::size_t>(tech)];
}

void TechTree::learn(Knowledge& k, int id, std::span<const int> techs) const {
    k.fields.resize(fields_.size());
    k.techs.resize(techCount_);
    k.fields.at(static_cast<std::size_t>(id)) = true;
    for (const int t : techs) {
        if (t >= 0 && static_cast<std::size_t>(t) < techCount_) {
            k.techs[static_cast<std::size_t>(t)] = true;
        }
    }
}

Knowledge TechTree::averageStart() const {
    Knowledge k;
    learn(k, kStartingTech, fields_[0].techs);
    for (const int f : prewarp_) {
        learn(k, f, field(f).techs);
    }
    return k;
}

int TechTree::researchable(const Knowledge& k, TechCategory category) const {
    int at = first(category);
    for (std::size_t steps = 0; at != 0 && steps < fields_.size(); ++steps) {
        if (!k.knowsField(at)) {
            return at;
        }
        const int next = fields_[static_cast<std::size_t>(at)].next;
        if (next == at) {
            return 0;
        }
        at = next;
    }
    return 0;
}

TechNames::TechNames(std::span<const std::uint8_t> entry, const TechTree& tree) : fields_(tree.fieldCount()) {
    words_.emplace_back();
    for (const auto b : entry) {
        if (b == 0) {
            words_.emplace_back();
        } else {
            words_.back().push_back(static_cast<char>(b));
        }
    }
    // The fields by id (the last ones, the hyper-advanced fields, are named
    // after their categories), "No Tech" as tech 0, then the techs.
    if (fields_ < kTechCategories || words_.size() < fields_ + tree.techCount()) {
        throw FormatError("TECHNAME.LBX: too few names");
    }
}

const std::string& TechNames::field(int id) const {
    return words_.at(static_cast<std::size_t>(id));
}

const std::string& TechNames::category(TechCategory category) const {
    if (category == TechCategory::Xenon) {
        return words_.at(0); // the file names no Xenon category; never shown
    }
    return words_.at(fields_ - (kTechCategories - 1) + static_cast<std::size_t>(category));
}

const std::string& TechNames::tech(int id) const {
    return words_.at(fields_ + static_cast<std::size_t>(id));
}

} // namespace hegemon
