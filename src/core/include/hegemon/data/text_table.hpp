// SPDX-License-Identifier: GPL-3.0-or-later
//
// Tables of fixed-size text records in LBX entries, such as the credits.
// See docs/formats/text.md.
#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace hegemon {

class TextTable {
public:
    // Throws FormatError when the records run past the end of the entry.
    static TextTable parse(std::span<const std::uint8_t> entry);

    std::size_t size() const { return records_.size(); }
    // Record `i` up to its first zero byte.
    const std::string& at(std::size_t i) const { return records_.at(i); }

private:
    std::vector<std::string> records_;
};

// An entry whose one record holds many texts, each ending at a zero byte,
// such as the messages of HESTRNGS.LBX. Throws FormatError when the record
// runs past the end of the entry.
std::vector<std::string> parseStringList(std::span<const std::uint8_t> entry);

// Fills a message's blanks as C's printf does, for what the original's
// messages use: %s, %d with a + flag, a 0 flag and a width, and %%. A blank
// without its value, or of the wrong kind, is left empty.
using MessageValue = std::variant<long long, std::string>;
std::string formatMessage(std::string_view pattern, std::initializer_list<MessageValue> values);

} // namespace hegemon
