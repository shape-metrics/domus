#include "domus/sat/sat.hpp"

#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

extern "C" {
#include "kissat/src/file.h"
#include "kissat/src/kissat.h"
#include "kissat/src/proof.h"
}

#include "../core/memory_file.hpp"
#include "domus/sat/cnf.hpp"

namespace domus::sat {
using namespace cnf;

class KissatSolver {
  private:
    kissat* m_solver = nullptr;
    std::string m_proof{};
    KissatSolver(kissat* solver) : m_solver(solver) {}

  public:
    static std::expected<KissatSolver, std::string> create() {
        kissat* solver = kissat_init();
        if (!solver)
            return std::unexpected("Failed to initialize Kissat solver");
        return KissatSolver(solver);
    }

    ~KissatSolver() {
        if (m_solver)
            kissat_release(m_solver);
    }

    void add_clause(const std::vector<int>& clause) {
        for (int lit : clause)
            kissat_add(m_solver, lit);
        kissat_add(m_solver, 0); // terminate clause
    }

    std::expected<bool, std::string> solve() {
        file proof_file;
        MemoryFile memory_file = MemoryFile::create().value();
        proof_file.file = memory_file.get_file();
        proof_file.close = true;
        proof_file.reading = false;
        proof_file.compressed = false;
        proof_file.path = NULL;
        proof_file.bytes = 0;
        kissat_init_proof(m_solver, &proof_file, false);
        int res = kissat_solve(m_solver);
        kissat_release_proof(m_solver);
        m_proof = memory_file.get_buffer();
        if (res == 10)
            return true;
        if (res == 20)
            return false;
        return std::unexpected("Kissat solver returned UNKNOWN");
    }

    bool value(int lit) const { return kissat_value(m_solver, lit) > 0; }

    const std::string& get_proof() const { return m_proof; }
};

SatSolverResult create_result(bool is_sat, KissatSolver& solver, const Cnf& cnf) {
    SatSolverResult result;
    if (is_sat) {
        result.result = SatSolverResultType::SAT;
        for (int var = 1; var <= static_cast<int>(cnf.get_number_of_variables()); ++var) {
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

std::expected<SatSolverResult, std::string> launch_kissat(const Cnf& cnf) {
    return KissatSolver::create().and_then(
        [&cnf](KissatSolver solver) -> std::expected<SatSolverResult, std::string> {
            for (const CnfRow& row : cnf.get_rows())
                if (row.type == CnfRowType::CLAUSE)
                    solver.add_clause(row.clause);
            return solver.solve().transform([&solver, &cnf](bool is_sat) -> SatSolverResult {
                return create_result(is_sat, solver, cnf);
            });
        }
    );
}

} // namespace domus::sat