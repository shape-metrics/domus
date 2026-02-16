#ifndef MY_SAT_H
#define MY_SAT_H

#include <string>
#include <vector>

class Cnf;

enum class SatSolverResultType { SAT, UNSAT };

struct SatSolverResult {
    SatSolverResultType result;
    std::vector<int> numbers;
    std::vector<std::string> proof_lines;
    [[nodiscard]] std::string to_string() const;
    void print() const;
};

SatSolverResult launch_glucose(const Cnf& cnf);

SatSolverResult launch_kissat(const Cnf& cnf);

#endif