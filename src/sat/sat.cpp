#include "domus/sat/sat.hpp"

#include <iostream>

std::string SatSolverResult::to_string() const {
    std::string r = result == SatSolverResultType::SAT ? "SAT" : "UNSAT";
    std::string numbers_str = "Numbers: ";
    for (int num : numbers)
        numbers_str += std::to_string(num) + " ";
    std::string proof_str = "Proof:\n";
    for (const std::string& line : proof_lines)
        proof_str += line + "\n";
    return r + "\n" + numbers_str + "\n" + proof_str;
}

void SatSolverResult::print() const { std::cout << to_string() << std::endl; }
