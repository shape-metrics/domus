#include "domus/sat/sat.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>

#include "domus/core/utils.hpp"
#include "domus/sat/cnf.hpp"

#include "glucose/src/SimpSolver.h"
#include "glucose/src/SolverTypes.h"
#include "glucose/src/Vec.h"

using namespace Glucose;

void readClause(const CnfRow& row, SimpSolver& S, vec<Lit>& lits) {
    int var;
    lits.clear();
    for (int lit : row.m_clause) {
        assert(lit != 0);
        var = abs(lit) - 1;
        while (var >= S.nVars())
            S.newVar();
        lits.push((lit > 0) ? mkLit(var) : ~mkLit(var));
    }
}

void parse_cnf(const Cnf& cnf, SimpSolver& S) {
    vec<Lit> lits;
    for (const CnfRow& row : cnf.get_rows()) {
        readClause(row, S, lits);
        S.addClause_(lits);
    }
}

void populate_proof_result(const char* buffer, SatSolverResult& result) {
    std::istringstream proof(buffer);
    std::string line;
    while (std::getline(proof, line))
        result.proof_lines.push_back(line);
}

SatSolverResult launch_glucose(const Cnf& cnf) {
    SimpSolver S;

    S.parsing = 1;
    S.use_simplification = true;
    S.verbosity = 0;
    S.verbEveryConflicts = 10000;
    S.showModel = false;

    MemoryFile memory_file_proof;

    S.certifiedUNSAT = true;
    S.vbyte = false;
    S.certifiedOutput = memory_file_proof.get_file();
    parse_cnf(cnf, S);

    S.parsing = 0;
    S.eliminate(true);

    SatSolverResult result;
    if (!S.okay()) {  // UNSAT
        fprintf(S.certifiedOutput, "0\n");
        result.result = SatSolverResultType::UNSAT;
        populate_proof_result(memory_file_proof.get_buffer(), result);
        return result;
    }

    vec<Lit> dummy;
    lbool ret = S.solveLimited(dummy);

    if (ret == l_True) {
        result.result = SatSolverResultType::SAT;
        for (int i = 0; i < S.nVars(); i++)
            if (S.model[i] != l_Undef)
                result.numbers.push_back(
                    (S.model[i] == l_True) ? i + 1 : -(i + 1));
    } else {
        result.result = SatSolverResultType::UNSAT;
        populate_proof_result(memory_file_proof.get_buffer(), result);
    }
    return result;
}