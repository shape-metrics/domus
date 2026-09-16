#include "domus/core/print.hpp"

#include "domus/core/debug.hpp"

namespace domus {

thread_local size_t print_depth = 0;

void increase_print_depth() { print_depth++; }

inline void decrease_print_depth() {
    DOMUS_ASSERT(print_depth > 0, "decrease_print_depth: print_depth is 0.");
    print_depth--;
}

ScopedPrintIndent::ScopedPrintIndent() { increase_print_depth(); }

ScopedPrintIndent::~ScopedPrintIndent() { decrease_print_depth(); }

// Number of dashes added per depth level (default 2 for double indentation, or 3 for extra
// emphasis)
constexpr size_t INDENT_STEP = 3;

// Generates the indentation prefix ("┣━━━━┫ ") or empty string if print_depth == 0
std::string get_indent_string() {
    if (print_depth == 0) {
        return "";
    }
    // Each depth level adds INDENT_STEP dashes:
    // depth 1 -> "┣━━┫ "
    // depth 2 -> "┣━━━━━┫ "
    // depth 3 -> "┣━━━━━━━━┫ "
    std::string indent = "┣";
    for (size_t i = 0; i < print_depth * INDENT_STEP - 1; ++i)
        indent += "━";
    indent += "┫ ";
    return indent;
}

} // namespace domus