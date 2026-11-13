// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/text_table.hpp"

#include "hegemon/data/reader.hpp"

namespace hegemon {

TextTable TextTable::parse(std::span<const std::uint8_t> entry) {
    Reader r(entry);
    const std::size_t count = r.u16();
    const std::size_t size = r.u16();
    if (size == 0 || count > r.remaining() / size) {
        throw FormatError("text records run past the end of the entry");
    }
    TextTable table;
    table.records_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto bytes = r.bytes(size);
        std::string text;
        for (const auto b : bytes) {
            if (b == 0) {
                break;
            }
            text.push_back(static_cast<char>(b));
        }
        table.records_.push_back(std::move(text));
    }
    return table;
}

std::vector<std::string> parseStringList(std::span<const std::uint8_t> entry) {
    Reader r(entry);
    const std::size_t count = r.u16();
    const std::size_t size = r.u16();
    if (count < 1 || size > r.remaining()) {
        throw FormatError("text records run past the end of the entry");
    }
    std::vector<std::string> out(1);
    for (const auto b : r.bytes(size)) {
        if (b == 0) {
            out.emplace_back();
        } else {
            out.back().push_back(static_cast<char>(b));
        }
    }
    while (!out.empty() && out.back().empty()) {
        out.pop_back(); // the zeros after the last text
    }
    return out;
}

} // namespace hegemon
