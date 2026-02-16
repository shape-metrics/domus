#include "domus/sat/sat.hpp"

#include <stdio.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "kissat/src/file.h"
#include "kissat/src/kissat.h"
#include "kissat/src/proof.h"
}

#include "domus/core/utils.hpp"
#include "domus/sat/cnf.hpp"

class KissatSolver {
   private:
    kissat* solver = nullptr;
    std::string proof{};

   public:
    KissatSolver() {
        solver = kissat_init();
        if (!solver)
            throw std::runtime_error("Failed to initialize Kissat solver");
    }

    ~KissatSolver() {
        if (solver)
            kissat_release(solver);
    }

    void add_clause(const std::vector<int>& clause) {
        for (int lit : clause)
            kissat_add(solver, lit);
        kissat_add(solver, 0);  // terminate clause
    }

    bool solve() {
        file proof_file;
        MemoryFile memory_file;
        proof_file.file = memory_file.get_file();
        proof_file.close = true;
        proof_file.reading = false;
        proof_file.compressed = false;
        proof_file.path = NULL;
        proof_file.bytes = 0;
        kissat_init_proof(solver, &proof_file, false);
        int res = kissat_solve(solver);
        kissat_release_proof(solver);
        proof = memory_file.get_buffer();
        if (res == 10)
            return true;
        if (res == 20)
            return false;
        throw std::runtime_error("Solver returned UNKNOWN");
    }

    bool value(int lit) const { return kissat_value(solver, lit) > 0; }

    const std::string& get_proof() const { return proof; }
};

SatSolverResult launch_kissat(const Cnf& cnf) {
    KissatSolver solver;
    for (const CnfRow& row : cnf.get_rows())
        if (row.m_type == CnfRowType::CLAUSE)
            solver.add_clause(row.m_clause);
    const bool is_sat = solver.solve();
    SatSolverResult result;
    if (is_sat) {
        result.result = SatSolverResultType::SAT;
        for (int var = 1; var <= cnf.get_number_of_variables(); ++var) {
            if (solver.value(var))
                result.numbers.push_back(var);
            else
                result.numbers.push_back(-var);
        }
    } else {
        result.result = SatSolverResultType::UNSAT;
        const std::string proof_str = solver.get_proof();
        std::istringstream iss(proof_str);
        std::string line;
        while (std::getline(iss, line))
            result.proof_lines.push_back(line);
    }
    return result;
}