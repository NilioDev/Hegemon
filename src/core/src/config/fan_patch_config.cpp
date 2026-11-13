// SPDX-License-Identifier: GPL-3.0-or-later
#include "hegemon/config/fan_patch_config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <system_error>

namespace hegemon {

namespace {

// Limits that keep damaged or hostile settings from running away.
constexpr std::uintmax_t kMaxFileSize = 1024 * 1024;
constexpr int kMaxDepth = 16;
constexpr std::size_t kMaxFiles = 1024;

std::string lower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool equalsNoCase(std::string_view a, std::string_view b) {
    return a.size() == b.size() && lower(a) == lower(b);
}

std::optional<std::int64_t> parseNumber(std::string_view w) {
    bool negative = false;
    std::size_t i = 0;
    if (!w.empty() && w[0] == '-') {
        negative = true;
        i = 1;
    }
    int base = 10;
    if (w.size() > i + 2 && w[i] == '0' && (w[i + 1] == 'x' || w[i + 1] == 'X')) {
        base = 16;
        i += 2;
    }
    if (i >= w.size() || w.size() - i > 15) {
        return std::nullopt;
    }
    std::int64_t v = 0;
    for (; i < w.size(); ++i) {
        const auto c = static_cast<unsigned char>(w[i]);
        int digit = 0;
        if (std::isdigit(c)) {
            digit = c - '0';
        } else if (base == 16 && std::isxdigit(c)) {
            digit = std::tolower(c) - 'a' + 10;
        } else {
            return std::nullopt;
        }
        v = v * base + digit;
    }
    return negative ? -v : v;
}

std::string where(std::string_view file, int line) {
    return std::string(file) + ", line " + std::to_string(line);
}

} // namespace

std::vector<ConfigStatement> parseConfig(std::string_view text, std::string_view file) {
    std::vector<ConfigStatement> out;
    ConfigStatement current;
    bool started = false;
    bool afterEquals = false;
    int line = 1;
    const auto add = [&](ConfigValue v, int at) {
        if (!started) {
            started = true;
            current.line = at;
        }
        if (afterEquals) {
            current.values.push_back(std::move(v));
        } else if (v.kind == ConfigValue::Kind::Text) {
            throw ConfigError(where(file, at) + ": a text before '='");
        } else {
            current.keys.push_back(v.text);
        }
    };
    std::size_t i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (c == '\n') {
            ++line;
            ++i;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
        } else if (c == '#') {
            while (i < text.size() && text[i] != '\n') {
                ++i;
            }
        } else if (c == ';') {
            if (started) {
                // A command keeps its arguments, as written, as values.
                if (!current.assignment && current.keys.size() > 1) {
                    for (std::size_t k = 1; k < current.keys.size(); ++k) {
                        current.values.push_back({ConfigValue::Kind::Word, current.keys[k], 0});
                    }
                    current.keys.resize(1);
                }
                for (auto& k : current.keys) {
                    k = lower(k);
                }
                out.push_back(std::move(current));
            }
            current = {};
            started = false;
            afterEquals = false;
            ++i;
        } else if (c == '=') {
            if (!started || afterEquals || current.keys.empty()) {
                throw ConfigError(where(file, line) + ": unexpected '='");
            }
            current.assignment = true;
            afterEquals = true;
            ++i;
        } else if (c == '"') {
            const int at = line;
            const auto end = text.find('"', i + 1);
            if (end == std::string_view::npos) {
                throw ConfigError(where(file, at) + ": text not closed");
            }
            const auto body = text.substr(i + 1, end - i - 1);
            line += static_cast<int>(std::count(body.begin(), body.end(), '\n'));
            add({ConfigValue::Kind::Text, std::string(body), 0}, at);
            i = end + 1;
        } else {
            const std::size_t start = i;
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])) && text[i] != ';' &&
                   text[i] != '=' && text[i] != '#' && text[i] != '"') {
                ++i;
            }
            const auto word = text.substr(start, i - start);
            ConfigValue v{ConfigValue::Kind::Word, std::string(word), 0};
            if (const auto n = parseNumber(word)) {
                v.kind = ConfigValue::Kind::Number;
                v.number = *n;
            }
            add(std::move(v), line);
        }
    }
    if (started) {
        throw ConfigError(where(file, current.line) + ": statement without ';'");
    }
    return out;
}

// Reads the main file, its includes and the enabled mods.
// Finds files named in settings: paths with `\\`, in any case, never
// outside the game folder and never through links.
class PathResolver {
public:
    PathResolver(std::filesystem::path root, std::string language)
        : root_(std::move(root)), language_(std::move(language)) {}

    // The real name in folder `dir` (from the root) matching `name` in any
    // case, as a file or a folder.
    std::optional<std::string> findName(const std::vector<std::string>& dir, std::string_view name,
                                        bool wantFolder) const {
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(join(dir), ec)) {
            if (e.is_symlink(ec)) {
                continue;
            }
            const bool ok = wantFolder ? e.is_directory(ec) : e.is_regular_file(ec);
            const auto n = e.path().filename().string();
            if (ok && equalsNoCase(n, name)) {
                return n;
            }
        }
        return std::nullopt;
    }

    std::filesystem::path join(const std::vector<std::string>& parts) const {
        auto p = root_;
        for (const auto& s : parts) {
            p /= s;
        }
        return p;
    }

    // A path from a settings file, as real names from the root. Throws when
    // it leaves the game folder.
    std::optional<std::vector<std::string>> resolve(const std::vector<std::string>& base, std::string path,
                                                    bool wantFolder, std::string_view file, int line) const {
        for (std::size_t at; (at = path.find("$LANG$")) != std::string::npos;) {
            path.replace(at, 6, language_);
        }
        std::vector<std::string> parts;
        std::size_t i = 0;
        if (!path.empty() && (path[0] == '\\' || path[0] == '/')) {
            i = 1;
        } else {
            parts = base;
        }
        std::vector<std::string> names;
        std::string piece;
        for (; i <= path.size(); ++i) {
            if (i == path.size() || path[i] == '\\' || path[i] == '/') {
                if (!piece.empty()) {
                    names.push_back(piece);
                }
                piece.clear();
            } else {
                piece.push_back(path[i]);
            }
        }
        for (std::size_t n = 0; n < names.size(); ++n) {
            const auto& name = names[n];
            if (name == ".") {
                continue;
            }
            if (name == "..") {
                if (parts.empty()) {
                    throw ConfigError(where(file, line) + ": path leaves the game folder");
                }
                parts.pop_back();
                continue;
            }
            const bool last = n + 1 == names.size();
            const auto real = findName(parts, name, last ? wantFolder : true);
            if (!real) {
                return std::nullopt;
            }
            parts.push_back(*real);
        }
        return parts;
    }

private:
    std::filesystem::path root_;
    std::string language_;
};

class ConfigLoader {
public:
    explicit ConfigLoader(FanPatchConfig& config) : config_(config), paths_(config.root_, config.language_) {}

    void run() {
        const auto main = paths_.findName({}, FanPatchConfig::kMainFile, false);
        if (!main) {
            throw ConfigError("unable to open " + std::string(FanPatchConfig::kMainFile) +
                              ": install the fan patch 1.51 in the game folder");
        }
        readFile({*main}, 0);
        scanMods();
        loadEnabledMods();
    }

    // Reads one file, its includes first where they are written.
    void readFile(const std::vector<std::string>& parts, int depth) {
        if (depth > kMaxDepth) {
            throw ConfigError("settings include each other too deeply");
        }
        if (++files_ > kMaxFiles) {
            throw ConfigError("too many settings files");
        }
        std::string name;
        for (const auto& p : parts) {
            name += (name.empty() ? "" : "\\") + p;
        }
        const auto text = readText(paths_.join(parts), name);
        std::vector<std::string> folder(parts.begin(), parts.end() - 1);
        for (auto& st : parseConfig(text, name)) {
            if (!st.assignment && !st.keys.empty() && st.keys[0] == "include") {
                for (const auto& v : st.values) {
                    include(folder, v.text, depth, name, st.line);
                }
            } else if (!st.assignment && !st.keys.empty() && st.keys[0] == "enable") {
                for (const auto& v : st.values) {
                    enabled_.push_back(lower(v.text));
                }
            } else {
                config_.entries_.push_back({std::move(st.keys), std::move(st.values), folder, name, st.line});
            }
        }
    }

    void include(const std::vector<std::string>& folder, std::string_view path, int depth, std::string_view file,
                 int line) {
        const bool optional = !path.empty() && path[0] == '?';
        const auto parts = paths_.resolve(folder, std::string(optional ? path.substr(1) : path), false, file, line);
        if (!parts) {
            if (optional) {
                return;
            }
            throw ConfigError(where(file, line) + ": unable to open " + std::string(path));
        }
        readFile(*parts, depth + 1);
    }

    static std::string readText(const std::filesystem::path& path, std::string_view name) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path, ec);
        if (ec || size > kMaxFileSize) {
            throw ConfigError("unable to read " + std::string(name));
        }
        std::ifstream in(path, std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!in && !in.eof()) {
            throw ConfigError("unable to read " + std::string(name));
        }
        return text;
    }

    struct Candidate {
        ConfigMod mod;
        std::vector<std::string> file;
        bool valid = true;
    };

    static std::int64_t defaultOrder(std::string_view modClass) {
        const auto c = lower(modClass);
        if (c == "core") {
            return 0;
        }
        if (c == "ai") {
            return 100;
        }
        if (c == "map") {
            return 200;
        }
        if (c == "ruleset") {
            return 300;
        }
        return 1000;
    }

    // Finds every mod file: the .CFG files one folder below `scan_mods`.
    void scanMods() {
        const ConfigEntry* scan = nullptr;
        for (const auto& e : config_.entries_) {
            if (e.keys.size() == 1 && e.keys[0] == "scan_mods") {
                scan = &e;
            }
        }
        if (scan == nullptr || scan->values.empty() || scan->values[0].text == "0") {
            return;
        }
        const auto dir = paths_.resolve(scan->folder, scan->values[0].text, true, scan->file, scan->line);
        if (!dir) {
            return;
        }
        std::error_code ec;
        std::vector<std::filesystem::path> files;
        for (const auto& sub : std::filesystem::directory_iterator(paths_.join(*dir), ec)) {
            if (sub.is_symlink(ec) || !sub.is_directory(ec)) {
                continue;
            }
            std::error_code ec2;
            for (const auto& f : std::filesystem::directory_iterator(sub.path(), ec2)) {
                const auto ext = lower(f.path().extension().string());
                if (!f.is_symlink(ec2) && f.is_regular_file(ec2) && ext == ".cfg") {
                    files.push_back(f.path());
                }
            }
        }
        std::sort(files.begin(), files.end());
        std::map<std::string, std::size_t> byId;
        for (const auto& f : files) {
            std::vector<std::string> parts = *dir;
            parts.push_back(f.parent_path().filename().string());
            parts.push_back(f.filename().string());
            const std::string name = f.parent_path().filename().string() + "\\" + f.filename().string();
            Candidate c;
            c.file = parts;
            c.mod.file = name;
            bool hasOrder = false;
            for (const auto& st : parseConfig(readText(f, name), name)) {
                if (!st.assignment || st.keys.size() != 1 || st.values.empty()) {
                    continue;
                }
                const auto& v = st.values[0];
                if (st.keys[0] == "mod_id") {
                    c.mod.id = v.text;
                } else if (st.keys[0] == "mod_name") {
                    c.mod.name = v.text;
                } else if (st.keys[0] == "mod_class") {
                    c.mod.modClass = v.text;
                } else if (st.keys[0] == "mod_order" && v.kind == ConfigValue::Kind::Number) {
                    c.mod.order = v.number;
                    hasOrder = true;
                }
            }
            if (c.mod.id.empty() || c.mod.modClass.empty()) {
                continue; // not a mod, such as a file another mod includes
            }
            if (!hasOrder) {
                c.mod.order = defaultOrder(c.mod.modClass);
            }
            if (c.mod.name.empty()) {
                c.mod.name = c.mod.id;
            }
            const auto id = lower(c.mod.id);
            if (const auto it = byId.find(id); it != byId.end()) {
                candidates_[it->second].valid = false; // two mods with one id: neither
                c.valid = false;
            } else {
                byId[id] = candidates_.size();
            }
            candidates_.push_back(std::move(c));
        }
    }

    // Loads the enabled mods by their order; loading one may enable more.
    void loadEnabledMods() {
        std::set<std::size_t> done;
        std::set<std::string> classes;
        for (;;) {
            std::optional<std::size_t> next;
            for (std::size_t i = 0; i < candidates_.size(); ++i) {
                const auto& c = candidates_[i];
                if (!c.valid || done.count(i) != 0 ||
                    std::find(enabled_.begin(), enabled_.end(), lower(c.mod.id)) == enabled_.end()) {
                    continue;
                }
                if (!next || c.mod.order < candidates_[*next].mod.order) {
                    next = i;
                }
            }
            if (!next) {
                return;
            }
            done.insert(*next);
            const auto& c = candidates_[*next];
            // Mods of one class are alternatives: the first one loaded wins.
            if (!classes.insert(lower(c.mod.modClass)).second) {
                continue;
            }
            config_.mods_.push_back(c.mod);
            readFile(c.file, 1);
        }
    }

private:
    FanPatchConfig& config_;
    PathResolver paths_;
    std::vector<std::string> enabled_;
    std::vector<Candidate> candidates_;
    std::size_t files_ = 0;
};

FanPatchConfig FanPatchConfig::load(const std::filesystem::path& gameFolder, std::string language) {
    FanPatchConfig config;
    config.root_ = gameFolder;
    config.language_ = std::move(language);
    ConfigLoader(config).run();
    return config;
}

const ConfigEntry* FanPatchConfig::last(std::initializer_list<std::string_view> keys) const {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->keys.size() == keys.size() && std::equal(keys.begin(), keys.end(), it->keys.begin())) {
            return &*it;
        }
    }
    return nullptr;
}

std::vector<const ConfigEntry*> FanPatchConfig::startingWith(std::initializer_list<std::string_view> keys) const {
    std::vector<const ConfigEntry*> out;
    for (const auto& e : entries_) {
        if (e.keys.size() >= keys.size() && std::equal(keys.begin(), keys.end(), e.keys.begin())) {
            out.push_back(&e);
        }
    }
    return out;
}

std::optional<std::filesystem::path> FanPatchConfig::resolve(const ConfigEntry& entry, std::string_view path) const {
    const bool optional = !path.empty() && path[0] == '?';
    const PathResolver loader(root_, language_);
    const auto parts =
        loader.resolve(entry.folder, std::string(optional ? path.substr(1) : path), false, entry.file, entry.line);
    if (!parts) {
        if (optional) {
            return std::nullopt;
        }
        throw ConfigError(where(entry.file, entry.line) + ": unable to open " + std::string(path));
    }
    return loader.join(*parts);
}

std::optional<std::filesystem::path> FanPatchConfig::lbxFile(std::string_view name) const {
    const auto key = lower(name);
    const auto* entry = last({key});
    if (entry == nullptr || entry->values.empty() || entry->values[0].text == "0") {
        return std::nullopt;
    }
    return resolve(*entry, entry->values[0].text);
}

} // namespace hegemon
