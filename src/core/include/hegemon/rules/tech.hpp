// SPDX-License-Identifier: GPL-3.0-or-later
//
// The technology tree: the fields of research, the applications each one
// gives, and what a player knows. Read from the fan patch's rule settings
// (tech_field, tech, starting_techfield, prewarp_techfield) and the names
// in TECHNAME.LBX. See docs/rules/research.md.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace hegemon {

class Parameters;

// The research categories, in the order of the settings' starting_techfield
// columns (and of their names in TECHNAME.LBX).
enum class TechCategory : std::uint8_t {
    Biology,
    Power,
    Physics,
    Construction,
    ForceFields,
    Chemistry,
    Computers,
    Sociology,
    Xenon
};
inline constexpr std::size_t kTechCategories = 9;

struct TechField {
    int id = 0;
    std::string key;      // the settings' name, such as advanced_biology
    int cost = 0;         // research points
    int next = 0;         // the field after it in its category, or 0
    int category = -1;    // a TechCategory, or -1 when in no category's chain
    std::vector<int> techs; // the applications it gives, in the settings' order
};

// What a player knows: by field id and by tech id.
struct Knowledge {
    std::vector<bool> fields;
    std::vector<bool> techs;

    bool knowsField(int id) const;
    bool knowsTech(int id) const;
};

class TechTree {
public:
    // Field 0, the technology every player starts with.
    static constexpr int kStartingTech = 0;

    // Throws std::out_of_range when a setting is missing and FormatError
    // when the fields don't form chains.
    explicit TechTree(const Parameters& parameters);

    // Field ids count from 0 (Starting Tech); tech ids from 1.
    std::size_t fieldCount() const { return fields_.size(); }
    std::size_t techCount() const { return techCount_; }
    const TechField& field(int id) const;
    // The tech's field, or -1 for one in no field.
    int techField(int tech) const;

    // The first field of a category.
    int first(TechCategory category) const { return first_[static_cast<std::size_t>(category)]; }

    // What a player knows at the start of a game with average technology:
    // Starting Tech and the pre-warp fields, with all their applications.
    Knowledge averageStart() const;
    // The field of a category that can be researched next: the first one in
    // its chain that isn't known. 0 when the whole chain is known.
    int researchable(const Knowledge& knowledge, TechCategory category) const;
    // Marks the field and the given applications known.
    void learn(Knowledge& knowledge, int field, std::span<const int> techs) const;

private:
    std::vector<TechField> fields_;
    std::vector<int> techFields_; // by tech id
    std::size_t techCount_ = 0;
    std::array<int, kTechCategories> first_{};
    std::vector<int> prewarp_;
};

// The names in TECHNAME.LBX's first entry: the fields by id (the
// hyper-advanced fields at the end carry their categories' names), then
// "No Tech" as tech 0 and the techs by id.
class TechNames {
public:
    // Throws FormatError when the entry has too few names for the tree.
    TechNames(std::span<const std::uint8_t> entry, const TechTree& tree);

    const std::string& field(int id) const;
    const std::string& category(TechCategory category) const;
    const std::string& tech(int id) const;

private:
    std::vector<std::string> words_;
    std::size_t fields_ = 0;
};

} // namespace hegemon
