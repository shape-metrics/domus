#pragma once

#ifndef NDEBUG

#include <print>  // IWYU pragma: keep
#include <ranges> // IWYU pragma: keep
#include <set>    // IWYU pragma: keep

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

#define DOMUS_DEBUG(message)                                                                       \
    do {                                                                                           \
        std::println("{}", message);                                                               \
    } while (0)

#define DOMUS_HAS_DUPLICATES(container)                                                            \
    ((container).size() !=                                                                         \
     (std::set<typename std::remove_cvref_t<decltype(container)>::value_type>(                     \
          (container).begin(),                                                                     \
          (container).end()                                                                        \
      ))                                                                                           \
         .size())

#else

#define DOMUS_ASSERT(condition, message, ...) ((void)0)

#define DOMUS_DEBUG(message) ((void)0)

#define DOMUS_HAS_DUPLICATES(container) (false)

#endif