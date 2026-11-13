// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/data/json.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>

namespace hegemon {

namespace {

constexpr int kMaxDepth = 64;
constexpr std::size_t kMaxSize = 64u * 1024u * 1024u;

void appendUtf8(std::string& out, std::uint32_t c) {
    if (c < 0x80) {
        out.push_back(static_cast<char>(c));
    } else if (c < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
}

} // namespace

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    JsonValue document() {
        JsonValue v = value(0);
        space();
        if (pos_ != text_.size()) {
            fail("text after the value");
        }
        return v;
    }

private:
    [[noreturn]] void fail(const char* what) const {
        throw FormatError(std::string("JSON: ") + what + " at byte " + std::to_string(pos_));
    }

    void space() {
        while (pos_ < text_.size() &&
               (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n' || text_[pos_] == '\r')) {
            ++pos_;
        }
    }

    char peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }

    void expect(std::string_view word) {
        if (text_.substr(pos_, word.size()) != word) {
            fail("unknown word");
        }
        pos_ += word.size();
    }

    JsonValue value(int depth) {
        if (depth > kMaxDepth) {
            fail("nested too deep");
        }
        space();
        JsonValue v;
        switch (peek()) {
        case '{': {
            ++pos_;
            auto object = std::make_shared<JsonValue::Object>();
            space();
            if (peek() == '}') {
                ++pos_;
            } else {
                for (;;) {
                    space();
                    if (peek() != '"') {
                        fail("expected a name");
                    }
                    std::string key = string();
                    space();
                    if (peek() != ':') {
                        fail("expected ':'");
                    }
                    ++pos_;
                    (*object)[std::move(key)] = value(depth + 1);
                    space();
                    if (peek() == ',') {
                        ++pos_;
                    } else if (peek() == '}') {
                        ++pos_;
                        break;
                    } else {
                        fail("expected ',' or '}'");
                    }
                }
            }
            v.value_ = std::move(object);
            break;
        }
        case '[': {
            ++pos_;
            auto array = std::make_shared<JsonValue::Array>();
            space();
            if (peek() == ']') {
                ++pos_;
            } else {
                for (;;) {
                    array->push_back(value(depth + 1));
                    space();
                    if (peek() == ',') {
                        ++pos_;
                    } else if (peek() == ']') {
                        ++pos_;
                        break;
                    } else {
                        fail("expected ',' or ']'");
                    }
                }
            }
            v.value_ = std::move(array);
            break;
        }
        case '"':
            v.value_ = string();
            break;
        case 't':
            expect("true");
            v.value_ = true;
            break;
        case 'f':
            expect("false");
            v.value_ = false;
            break;
        case 'n':
            expect("null");
            break;
        default:
            v.value_ = number();
            break;
        }
        return v;
    }

    std::uint32_t hex4() {
        if (text_.size() - pos_ < 4) {
            fail("short \\u escape");
        }
        std::uint32_t c = 0;
        for (int i = 0; i < 4; ++i) {
            const char h = text_[pos_++];
            c <<= 4;
            if (h >= '0' && h <= '9') {
                c |= static_cast<std::uint32_t>(h - '0');
            } else if (h >= 'a' && h <= 'f') {
                c |= static_cast<std::uint32_t>(h - 'a' + 10);
            } else if (h >= 'A' && h <= 'F') {
                c |= static_cast<std::uint32_t>(h - 'A' + 10);
            } else {
                fail("bad \\u escape");
            }
        }
        return c;
    }

    std::string string() {
        ++pos_; // the opening quote
        std::string out;
        for (;;) {
            if (pos_ >= text_.size()) {
                fail("text not closed");
            }
            const char c = text_[pos_++];
            if (c == '"') {
                return out;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                fail("control character in a text");
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (pos_ >= text_.size()) {
                fail("text not closed");
            }
            switch (text_[pos_++]) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u': {
                std::uint32_t cp = hex4();
                if (cp >= 0xD800 && cp < 0xDC00 && text_.substr(pos_, 2) == "\\u") {
                    pos_ += 2;
                    const std::uint32_t low = hex4();
                    if (low < 0xDC00 || low >= 0xE000) {
                        fail("bad surrogate pair");
                    }
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                }
                appendUtf8(out, cp);
                break;
            }
            default:
                fail("unknown escape");
            }
        }
    }

    double number() {
        const std::size_t start = pos_;
        if (peek() == '-') {
            ++pos_;
        }
        const auto digits = [&] {
            const std::size_t from = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                ++pos_;
            }
            return pos_ > from;
        };
        if (!digits()) {
            fail("expected a value");
        }
        if (peek() == '.') {
            ++pos_;
            if (!digits()) {
                fail("bad number");
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++pos_;
            if (peek() == '+' || peek() == '-') {
                ++pos_;
            }
            if (!digits()) {
                fail("bad number");
            }
        }
        const std::string copy(text_.substr(start, pos_ - start));
        const double v = std::strtod(copy.c_str(), nullptr);
        if (!std::isfinite(v)) {
            fail("number too big");
        }
        return v;
    }

    std::string_view text_;
    std::size_t pos_ = 0;
};

JsonValue JsonValue::parse(std::string_view text) {
    if (text.size() > kMaxSize) {
        throw FormatError("JSON: file too big");
    }
    return JsonParser(text).document();
}

bool JsonValue::boolean() const {
    if (!isBool()) {
        throw FormatError("JSON: expected true or false");
    }
    return std::get<bool>(value_);
}

double JsonValue::number() const {
    if (!isNumber()) {
        throw FormatError("JSON: expected a number");
    }
    return std::get<double>(value_);
}

std::int64_t JsonValue::integer() const {
    const double v = number();
    // 2^63 as a double; anything from there on doesn't fit.
    constexpr double kLimit = 9223372036854775808.0;
    if (v != std::floor(v) || v >= kLimit || v < -kLimit) {
        throw FormatError("JSON: expected a whole number");
    }
    return static_cast<std::int64_t>(v);
}

const std::string& JsonValue::string() const {
    if (!isString()) {
        throw FormatError("JSON: expected a text");
    }
    return std::get<std::string>(value_);
}

const JsonValue::Array& JsonValue::array() const {
    if (!isArray()) {
        throw FormatError("JSON: expected a list");
    }
    return *std::get<std::shared_ptr<Array>>(value_);
}

const JsonValue::Object& JsonValue::object() const {
    if (!isObject()) {
        throw FormatError("JSON: expected an object");
    }
    return *std::get<std::shared_ptr<Object>>(value_);
}

const JsonValue* JsonValue::find(std::string_view key) const {
    if (!isObject()) {
        return nullptr;
    }
    const auto& o = object();
    const auto it = o.find(key);
    return it == o.end() ? nullptr : &it->second;
}

} // namespace hegemon
