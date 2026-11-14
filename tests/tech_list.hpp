// SPDX-License-Identifier: GPL-3.0-or-later
//
// A small technology tree in the fan patch's layout, for tests: each
// category c has two fields, c + 1 (cost 50) and then c + 10 (cost 80);
// Xenon (9) leads to itself. Construction's first field (4) is pre-warp.
// Tech 1 is Starting Tech's, tech 2 the first construction field's, techs 3
// and 4 the second's.
#pragma once

#include <string>

namespace hegemon::test {

inline std::string techSettings() {
    std::string names = "\"starting_tech\"";
    std::string rows, defaults;
    for (int id = 1; id <= 18; ++id) {
        const std::string name = "field" + std::to_string(id);
        names += ",\"" + name + "\"";
        rows += std::string(id > 1 ? "," : "") + "\"" + name + "\"";
        const int next = id == 9 ? 9 : (id < 10 ? id + 9 : 0);
        const int cost = id == 9 ? 15000 : (id < 10 ? 50 : 80);
        defaults += std::string(id > 1 ? "," : "") + "[" + std::to_string(id) + ",0," + std::to_string(next) + "," +
                    std::to_string(cost) + ",0]";
    }
    const char* categories[] = {"biology", "power", "physics", "construction", "force_fields",
                                "chemistry", "computers", "sociology", "xenon"};
    std::string columns, starts;
    for (int c = 0; c < 9; ++c) {
        columns += std::string(c > 0 ? "," : "") + "{\"name\": \"" + categories[c] +
                   "_field1\", \"type\": \"enum\", \"values\": [" + names + "]}";
        starts += std::string(c > 0 ? "," : "") + "\"field" + std::to_string(c + 1) + "\"";
    }
    return R"( {"name": "tech_field", "type": "table", "rows": [[)" + rows + R"(]], "default": [)" + defaults +
           R"(], "columns": [{"name": "id"}, {"name": "prev_field_id"}, {"name": "next_field_id"}, {"name": "cost"}, {"name": "AI_group"}]},
 {"name": "tech", "type": "table", "rows": [["t1", "t2", "t3", "t4"]], "default": [[1, 0, 0, 0], [2, 4, 0, 0], [3, 13, 0, 0], [4, 13, 0, 0]],
  "columns": [{"name": "id"}, {"name": "tech_field_id"}, {"name": "AI_group"}, {"name": "in_strategic"}]},
 {"name": "starting_techfield", "type": "table", "rows": [], "default": [)" +
           starts + R"(], "columns": [)" + columns + R"(]},
 {"name": "prewarp_techfield", "type": "table", "rows": [], "default": ["field4"], "columns": [{"name": "pw_field1", "type": "enum", "values": [)" +
           names + R"(]}]},
 {"name": "fixed_research_cost", "type": "integer", "min": 0, "max": 1, "default": 0},
 {"name": "keep_research_on_breakthrough", "type": "integer", "min": 0, "max": 1, "default": 0})";
}

} // namespace hegemon::test
