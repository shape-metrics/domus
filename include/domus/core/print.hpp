#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <print> // IWYU pragma: keep
#include <string>
#include <string_view>
#include <utility>

namespace domus {

extern thread_local size_t print_depth;

void increase_print_depth();

void decrease_print_depth();

struct ScopedPrintIndent {
    ScopedPrintIndent();
    ~ScopedPrintIndent();
    ScopedPrintIndent(const ScopedPrintIndent&) = delete;
    ScopedPrintIndent& operator=(const ScopedPrintIndent&) = delete;
};

std::string get_indent_string();

inline std::string_view strip_indent_prefix(std::string_view sv) {
    constexpr std::string_view box_start = "┣";
    constexpr std::string_view dash = "━";
    constexpr std::string_view box_end = "┫";

    while (!sv.empty()) {
        if (print_depth > 0) {
            std::string prefix = get_indent_string();
            if (!prefix.empty() && sv.starts_with(prefix)) {
                sv.remove_prefix(prefix.size());
                continue;
            }
        }
        if (sv.starts_with(box_start)) {
            size_t pos = box_start.size();
            size_t dash_count = 0;
            while (pos + dash.size() <= sv.size() && sv.substr(pos, dash.size()) == dash) {
                pos += dash.size();
                ++dash_count;
            }
            if (pos + box_end.size() <= sv.size() && sv.substr(pos, box_end.size()) == box_end) {
                pos += box_end.size();
            }
            if (dash_count > 0 && pos < sv.size() && sv[pos] == ' ') {
                sv.remove_prefix(pos + 1);
                continue;
            }
        }
        break;
    }
    return sv;
}

template <typename T>
    requires std::convertible_to<T, std::string_view>
std::string_view clean_arg(T&& arg) {
    return strip_indent_prefix(std::string_view(arg));
}

template <typename T>
    requires (!std::convertible_to<T, std::string_view>)
decltype(auto) clean_arg(T&& arg) {
    return std::forward<T>(arg);
}

template <typename... Args>
std::string format_cleaned(std::format_string<Args...> fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        return std::string(fmt.get());
    } else {
        auto format_helper = [&](auto&&... cleaned) {
            return std::vformat(fmt.get(), std::make_format_args(cleaned...));
        };
        return format_helper(clean_arg(std::forward<Args>(args))...);
    }
}

// Type-safe format returning std::string with current depth prefix
template <typename... Args> std::string format(std::format_string<Args...> fmt, Args&&... args) {
    std::string msg = format_cleaned(fmt, std::forward<Args>(args)...);
    if (print_depth > 0) {
        std::string prefix = get_indent_string();
        if (!msg.starts_with(prefix))
            return prefix + msg;
    }
    return msg;
}

// Type-safe format_to inserting into any output iterator (e.g. std::back_inserter)
template <typename OutputIt, typename... Args>
OutputIt format_to(OutputIt out, std::format_string<Args...> fmt, Args&&... args) {
    std::string msg = format_cleaned(fmt, std::forward<Args>(args)...);
    if (print_depth > 0) {
        std::string prefix = get_indent_string();
        if (!msg.starts_with(prefix))
            out = std::format_to(out, "{}", prefix);
    }
    return std::format_to(out, "{}", msg);
}

template <typename... Args> void print(std::format_string<Args...> fmt, Args&&... args) {
    std::string msg = format_cleaned(fmt, std::forward<Args>(args)...);
    if (print_depth > 0) {
        std::string prefix = get_indent_string();
        if (!msg.starts_with(prefix))
            std::print("{}", prefix);
        std::print("{}", msg);
    } else
        std::print("{}", msg);
}

template <typename... Args> void println(std::format_string<Args...> fmt, Args&&... args) {
    std::string msg = format_cleaned(fmt, std::forward<Args>(args)...);
    if (print_depth > 0) {
        std::string prefix = get_indent_string();
        if (!msg.starts_with(prefix))
            std::print("{}", prefix);
        std::println("{}", msg);
    } else
        std::println("{}", msg);
}

} // namespace domus
