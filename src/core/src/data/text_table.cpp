// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/text_table.hpp"

#include "hegemon/data/reader.hpp"

namespace hegemon {

TextTable TextTable::parse(std::span<const std::uint8_t> entry, std::size_t offset) {
    Reader r(entry);
    const std::size_t count = r.u16();
    const std::size_t size = r.u16();
    if (size == 0 || count > r.remaining() / size) {
        throw FormatError("text records run past the end of the entry");
    }
    if (offset >= size) {
        throw FormatError("text records are too short");
    }
    TextTable table;
    table.records_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto bytes = r.bytes(size).subspan(offset);
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

std::string formatMessage(std::string_view pattern, std::initializer_list<MessageValue> values) {
    std::string out;
    auto next = values.begin();
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] != '%' || i + 1 >= pattern.size()) {
            out.push_back(pattern[i]);
            continue;
        }
        ++i;
        if (pattern[i] == '%') {
            out.push_back('%');
            continue;
        }
        bool plus = false, zero = false;
        for (; i < pattern.size() && (pattern[i] == '+' || pattern[i] == '0'); ++i) {
            plus = plus || pattern[i] == '+';
            zero = zero || pattern[i] == '0';
        }
        std::size_t width = 0;
        for (; i < pattern.size() && pattern[i] >= '0' && pattern[i] <= '9' && width < 100; ++i) {
            width = width * 10 + static_cast<std::size_t>(pattern[i] - '0');
        }
        if (i >= pattern.size()) {
            break;
        }
        const char kind = pattern[i];
        const MessageValue* value = next != values.end() ? &*next++ : nullptr;
        std::string text;
        if (kind == 'd' && value != nullptr && std::holds_alternative<long long>(*value)) {
            const long long n = std::get<long long>(*value);
            text = std::to_string(n < 0 ? -n : n);
            const std::string sign = n < 0 ? "-" : (plus ? "+" : "");
            if (zero && text.size() + sign.size() < width) {
                text.insert(0, width - text.size() - sign.size(), '0');
            }
            text.insert(0, sign);
        } else if (kind == 's' && value != nullptr && std::holds_alternative<std::string>(*value)) {
            text = std::get<std::string>(*value);
        }
        if (text.size() < width) {
            text.insert(0, width - text.size(), ' ');
        }
        out += text;
    }
    return out;
}

} // namespace hegemon
