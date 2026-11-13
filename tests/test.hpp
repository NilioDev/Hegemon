// SPDX-License-Identifier: GPL-3.0-or-later
//
// A deliberately tiny test harness, so the core can be tested without
// third-party dependencies.
//
//   HGM_TEST(name) { HGM_CHECK(1 + 1 == 2); }
#pragma once

#include <functional>
#include <string_view>
#include <vector>

namespace hegemon::test {

struct Case {
    std::string_view name;
    std::function<void()> body;
};

std::vector<Case>& registry();
void reportFailure(const char* file, int line, const char* expression);
void reportSkip(const char* reason);

struct Registrar {
    Registrar(std::string_view name, std::function<void()> body) {
        registry().push_back({name, std::move(body)});
    }
};

} // namespace hegemon::test

#define HGM_TEST_CONCAT_INNER(a, b) a##b
#define HGM_TEST_CONCAT(a, b) HGM_TEST_CONCAT_INNER(a, b)

#define HGM_TEST(name)                                                                     \
    static void HGM_TEST_CONCAT(hgm_test_fn_, __LINE__)();                                 \
    static const ::hegemon::test::Registrar HGM_TEST_CONCAT(hgm_test_reg_, __LINE__){          \
        name, &HGM_TEST_CONCAT(hgm_test_fn_, __LINE__)};                                   \
    static void HGM_TEST_CONCAT(hgm_test_fn_, __LINE__)()

#define HGM_CHECK(...)                                                              \
    do {                                                                                   \
        if (!(__VA_ARGS__)) {                                                               \
            ::hegemon::test::reportFailure(__FILE__, __LINE__, #__VA_ARGS__);                   \
        }                                                                                  \
    } while (false)

// Ends the current test early and reports it as skipped, for tests that need
// files which are not always present (such as the original game data).
#define HGM_SKIP(reason)                                                                   \
    do {                                                                                   \
        ::hegemon::test::reportSkip(reason);                                                   \
        return;                                                                            \
    } while (false)
