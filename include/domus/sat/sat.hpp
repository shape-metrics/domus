#pragma once

#include <expected>
#include <string>
#include <vector>

namespace domus::sat {
namespace cnf {
class Cnf;
}

enum class SatSolverResultType { SAT, UNSAT };

struct SatSolverResult {
    SatSolverResultType result;
    std::vector<int> numbers;
    std::vector<std::string> proof_lines;
    std::string to_string() const;
    void print() const;
};

SatSolverResult launch_glucose(const cnf::Cnf& cnf);

std::expected<SatSolverResult, std::string> launch_kissat(const cnf::Cnf& cnf);

} // namespace domus::sat