#include "clauses_functions.hpp"

#include <cstddef>
#include <stddef.h>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/shape/direction.hpp"
#include "domus/sat/cnf.hpp"

#include "../../core/domus_debug.hpp"
#include "variables_handler.hpp"

namespace domus::orthogonal::shape {
using namespace sat::cnf;
using namespace sat;
using namespace graph;

int get_variable(
    const Graph& graph,
    const VariablesHandler& handler,
    const size_t node_id,
    const size_t neighbor_id,
    const size_t edge_id,
    const Direction direction
) {
    auto [from_id, to_id] = graph.get_edge(edge_id);
    if (from_id == node_id && to_id == neighbor_id)
        return static_cast<int>(handler.get_variable(edge_id, direction));
    if (from_id == neighbor_id && to_id == node_id)
        return static_cast<int>(handler.get_variable(edge_id, opposite_direction(direction)));
    DOMUS_ASSERT(false, "get_variable: error");
    return 0;
}

void add_constraints_at_most_one_is_true(
    Cnf& cnf_builder, int var_1, int var_2, int var_3, int var_4
) {
    // at most one is true (at least three are false)
    // for every possible pair, at least one is false
    cnf_builder.add_clause({-var_1, -var_2});
    cnf_builder.add_clause({-var_1, -var_3});
    cnf_builder.add_clause({-var_1, -var_4});
    cnf_builder.add_clause({-var_2, -var_3});
    cnf_builder.add_clause({-var_2, -var_4});
    cnf_builder.add_clause({-var_3, -var_4});
}

void add_constraints_one_direction_per_edge(
    Cnf& cnf_builder, int up, int down, int right, int left
) {
    cnf_builder.add_clause({up, down, right, left}); // at least one is true
    add_constraints_at_most_one_is_true(cnf_builder, up, down, left, right);
}

void add_constraints_one_direction_per_edge(
    const Graph& graph, Cnf& cnf_builder, const VariablesHandler& handler
) {

    graph.for_each_node([&](size_t node_id_1) {
        graph.for_each_out_edge(node_id_1, [&](EdgeIter edge) {
            int up = static_cast<int>(handler.get_up_variable(edge.id));
            int down = static_cast<int>(handler.get_down_variable(edge.id));
            int right = static_cast<int>(handler.get_right_variable(edge.id));
            int left = static_cast<int>(handler.get_left_variable(edge.id));
            add_constraints_one_direction_per_edge(cnf_builder, up, down, right, left);
        });
    });
}

void add_clause_at_least_one_in_direction(
    const Graph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    size_t node_id,
    Direction direction
) {
    std::vector<int> clause;
    graph.for_each_edge(node_id, [&](EdgeIter edge) {
        int variable = get_variable(graph, handler, node_id, edge.neighbor_id, edge.id, direction);
        clause.push_back(variable);
    });
    cnf_builder.add_clause(clause);
}

void add_one_edge_per_direction_clauses(
    const Graph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    const Direction direction,
    size_t node_id
) {
    size_t degree = graph.get_degree_of_node(node_id);
    if (degree == 4) {
        add_clause_at_least_one_in_direction(graph, cnf_builder, handler, node_id, direction);
    } else if (degree == 3) {
        std::vector<int> variables;
        graph.for_each_edge(node_id, [&](EdgeIter edge) {
            int variable =
                get_variable(graph, handler, node_id, edge.neighbor_id, edge.id, direction);

            variables.push_back(variable);
        });
        // at most one is true (at least 2 are false)
        cnf_builder.add_clause({-variables[0], -variables[1]});
        cnf_builder.add_clause({-variables[0], -variables[2]});
        cnf_builder.add_clause({-variables[1], -variables[2]});
    } else if (degree == 2) {
        std::vector<int> clause;
        graph.for_each_edge(node_id, [&](EdgeIter edge) {
            int variable =
                get_variable(graph, handler, node_id, edge.neighbor_id, edge.id, direction);
            clause.push_back(-variable);
        });
        // at most one is true (at least 1 is false)
        cnf_builder.add_clause(clause);
    } else if (degree != 1) {
        DOMUS_ASSERT(
            false,
            "add_one_edge_per_direction_clauses: internal error, degree of node is not valid"
        );
    }
}

void add_cycles_constraints(
    const Graph& graph,
    Cnf& cnf_builder,
    const std::vector<graph::Cycle>& cycles,
    const VariablesHandler& handler
) {
    for (const graph::Cycle& cycle : cycles) {
        std::vector<int> at_least_one_down{};
        std::vector<int> at_least_one_up{};
        std::vector<int> at_least_one_right{};
        std::vector<int> at_least_one_left{};
        for (size_t i = 0; i < cycle.size(); i++) {
            size_t cycle_node = cycle.node_id_at(i);
            size_t next_cycle_node = cycle.node_id_at(i + 1);
            size_t cycle_edge_id = cycle.edge_id_at(i);
            DOMUS_ASSERT(
                graph.are_neighbors(cycle_node, next_cycle_node),
                "add_cycles_constraints: cycle nodes are not neighbors"
            );
            at_least_one_down.push_back(get_variable(
                graph,
                handler,
                cycle_node,
                next_cycle_node,
                cycle_edge_id,
                Direction::DOWN
            ));
            at_least_one_up.push_back(get_variable(
                graph,
                handler,
                cycle_node,
                next_cycle_node,
                cycle_edge_id,
                Direction::UP
            ));
            at_least_one_right.push_back(get_variable(
                graph,
                handler,
                cycle_node,
                next_cycle_node,
                cycle_edge_id,
                Direction::RIGHT
            ));
            at_least_one_left.push_back(get_variable(
                graph,
                handler,
                cycle_node,
                next_cycle_node,
                cycle_edge_id,
                Direction::LEFT
            ));
        }
        cnf_builder.add_clause(at_least_one_down);
        cnf_builder.add_clause(at_least_one_up);
        cnf_builder.add_clause(at_least_one_right);
        cnf_builder.add_clause(at_least_one_left);
    }
}

void add_nodes_constraints(const Graph& graph, Cnf& cnf_builder, const VariablesHandler& handler) {
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) <= 4) {
            add_one_edge_per_direction_clauses(graph, cnf_builder, handler, Direction::UP, node_id);
            add_one_edge_per_direction_clauses(
                graph,
                cnf_builder,
                handler,
                Direction::DOWN,
                node_id
            );
            add_one_edge_per_direction_clauses(
                graph,
                cnf_builder,
                handler,
                Direction::RIGHT,
                node_id
            );
            add_one_edge_per_direction_clauses(
                graph,
                cnf_builder,
                handler,
                Direction::LEFT,
                node_id
            );
        } else {
            add_clause_at_least_one_in_direction(
                graph,
                cnf_builder,
                handler,
                node_id,
                Direction::UP
            );
            add_clause_at_least_one_in_direction(
                graph,
                cnf_builder,
                handler,
                node_id,
                Direction::DOWN
            );
            add_clause_at_least_one_in_direction(
                graph,
                cnf_builder,
                handler,
                node_id,
                Direction::RIGHT
            );
            add_clause_at_least_one_in_direction(
                graph,
                cnf_builder,
                handler,
                node_id,
                Direction::LEFT
            );
        }
    });
}

} // namespace domus::orthogonal::shape