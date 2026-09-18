#pragma once

#ifndef NDEBUG

#include <print>  // IWYU pragma: keep
#include <ranges> // IWYU pragma: keep
#include <set>    // IWYU pragma: keep

#include "print.hpp" // IWYU pragma: keep

#define DOMUS_ASSERT(condition, message, ...)                                                      \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::println(                                                                          \
                stderr,                                                                            \
                "Assertion failed: " message "\nFile: {}, Line: {}",                               \
                __VA_ARGS__ __VA_OPT__(, ) __FILE__,                                               \
                __LINE__                                                                           \
            );                                                                                     \
            std::terminate();                                                                      \
        }                                                                                          \
    } while (0)

#define DOMUS_DEBUG_LN(message, ...)                                                               \
    do {                                                                                           \
        domus::println(message __VA_OPT__(, ) __VA_ARGS__);                                        \
    } while (0)

#define DOMUS_DEBUG(message, ...)                                                                  \
    do {                                                                                           \
        domus::print(message __VA_OPT__(, ) __VA_ARGS__);                                          \
    } while (0)

#define DOMUS_CONCAT_IMPL(a, b) a##b
#define DOMUS_CONCAT(a, b) DOMUS_CONCAT_IMPL(a, b)

#define DOMUS_DEBUG_INDENT()                                                                       \
    [[maybe_unused]] ::domus::ScopedPrintIndent DOMUS_CONCAT(_domus_indent_, __LINE__)

#else

#define DOMUS_ASSERT(condition, message, ...) ((void)0)

#define DOMUS_DEBUG_LN(message, ...) ((void)0)

#define DOMUS_DEBUG(message, ...) ((void)0)

#define DOMUS_DEBUG_INDENT() ((void)0)

#endif
