#include "domus/sat/sat.hpp"

#include <print>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/sat/cnf.hpp"

#include "../core/domus_debug.hpp"

namespace domus::sat {
using namespace graph;

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

size_t variable_to_node_id(int variable) {
    DOMUS_ASSERT(variable != 0, "variable_to_node_id: variable cannot be 0");
    int abs_variable = 2 * (std::abs(variable) - 1) + (variable < 0);
    return static_cast<size_t>(abs_variable);
}

SatSolverResult solve_2_sat(const cnf::Cnf& cnf) {
    const size_t num_vars = cnf.get_number_of_variables();
    Graph graph;
    for (size_t i = 0; i < num_vars; ++i) {
        graph.add_node();
        graph.add_node();
    }
    for (const cnf::CnfRow& row : cnf.get_rows()) {
        DOMUS_ASSERT(
            row.clause.size() <= 2,
            "solve_2_sat: clause cannot have more than 2 literals"
        );
        if (row.clause.size() == 1) {
            graph.add_edge(variable_to_node_id(-row.clause[0]), variable_to_node_id(row.clause[0]));
        }
        if (row.clause.size() == 2) {
            graph.add_edge(variable_to_node_id(-row.clause[0]), variable_to_node_id(row.clause[1]));
            graph.add_edge(variable_to_node_id(-row.clause[1]), variable_to_node_id(row.clause[0]));
        }
    }

    algorithms::StrongConnectedComponents scc =
        algorithms::StrongConnectedComponents::compute(graph);

    SatSolverResult result;
    result.numbers.resize(num_vars);

    for (size_t i = 0; i < num_vars; ++i) {
        size_t pos_node = variable_to_node_id(static_cast<int>(i + 1));
        size_t neg_node = variable_to_node_id(-static_cast<int>(i + 1));

        size_t scc_pos = scc.node_to_scc_id.get_label(pos_node);
        size_t scc_neg = scc.node_to_scc_id.get_label(neg_node);

        // if x e !x are in the same SCC, there is no solution
        if (scc_pos == scc_neg) {
            result.result = SatSolverResultType::UNSAT;
            result.numbers.clear();
            return result;
        }

        if (scc_pos > scc_neg)
            result.numbers[i] = static_cast<int>(i + 1); // x_i is TRUE
        else
            result.numbers[i] = -static_cast<int>(i + 1); // x_i is FALSE
    }

    result.result = SatSolverResultType::SAT;
    return result;
}

} // namespace domus::sat