#include "domus/sat/sat.hpp"

#include <print>

std::string SatSolverResult::to_string() const {
    std::string result_str;
    auto out = std::back_inserter(result_str);
    std::format_to(out, "{}\n", (result == SatSolverResultType::SAT ? "SAT" : "UNSAT"));
    std::format_to(out, "Numbers: ");
    for (int num : numbers)
        std::format_to(out, "{} ", num);
    std::format_to(out, "\nProof:\n");
    for (const auto& line : proof_lines)
        std::format_to(out, "{}\n", line);
    return result_str;
}

void SatSolverResult::print() const { std::print("{}", to_string()); }
