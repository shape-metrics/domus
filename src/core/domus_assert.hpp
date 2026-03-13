#pragma once

#ifndef NDEBUG

#include <print> // IWYU pragma: keep

#define DOMUS_ASSERT(condition, message)                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::println(                                                                          \
                stderr,                                                                            \
                "Assertion failed: {}\nFile: {}, Line: {}",                                        \
                message,                                                                           \
                __FILE__,                                                                          \
                __LINE__                                                                           \
            );                                                                                     \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

#else
#define DOMUS_ASSERT(condition, message) ((void)0)
#endif