#pragma once

#ifndef NDEBUG

#include <print> // IWYU pragma: keep

#define DOMUS_ASSERT(condition, message, ...)                                                      \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::println(                                                                          \
                stderr,                                                                            \
                "Assertion failed: " message "\nFile: {}, Line: {}",                               \
                __VA_ARGS__ __VA_OPT__(, ) __FILE__,                                               \
                __LINE__                                                                           \
            );                                                                                     \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

#define DOMUS_DEBUG(message)                                                                       \
    do {                                                                                           \
        std::println("{}", message);                                                               \
    } while (0)

#else

#define DOMUS_ASSERT(condition, message) ((void)0)

#define DOMUS_DEBUG(message) ((void)0)

#endif