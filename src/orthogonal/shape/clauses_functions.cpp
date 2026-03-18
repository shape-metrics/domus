#include "clauses_functions.hpp"

#include <cstddef>
#include <stddef.h>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/sat/cnf.hpp"

#include "../../core/domus_assert.hpp"
#include "variables_handler.hpp"

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
        graph.for_each_neighbor(node_id_1, [&](size_t node_id_2) {
            if (node_id_1 > node_id_2)
                return;
            int up = static_cast<int>(handler.get_up_variable(node_id_1, node_id_2));
            int down = static_cast<int>(handler.get_down_variable(node_id_1, node_id_2));
            int right = static_cast<int>(handler.get_right_variable(node_id_1, node_id_2));
            int left = static_cast<int>(handler.get_left_variable(node_id_1, node_id_2));
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
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        size_t variable = handler.get_variable(node_id, neighbor_id, direction);
        clause.push_back(static_cast<int>(variable));
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
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            size_t variable = handler.get_variable(node_id, neighbor_id, direction);
            variables.push_back(static_cast<int>(variable));
        });
        // at most one is true (at least 2 are false)
        cnf_builder.add_clause({-variables[0], -variables[1]});
        cnf_builder.add_clause({-variables[0], -variables[2]});
        cnf_builder.add_clause({-variables[1], -variables[2]});
    } else if (degree == 2) {
        std::vector<int> clause;
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            size_t variable = handler.get_variable(node_id, neighbor_id, direction);
            clause.push_back(-static_cast<int>(variable));
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
    Cnf& cnf_builder, const std::vector<Cycle>& cycles, const VariablesHandler& handler
) {
    for (const Cycle& cycle : cycles) {
        std::vector<int> at_least_one_down{};
        std::vector<int> at_least_one_up{};
        std::vector<int> at_least_one_right{};
        std::vector<int> at_least_one_left{};
        for (size_t i = 0; i < cycle.size(); i++) {
            size_t cycle_node = cycle[i];
            size_t next_cycle_node = cycle[i + 1];
            at_least_one_down.push_back(
                static_cast<int>(handler.get_down_variable(cycle_node, next_cycle_node))
            );
            at_least_one_up.push_back(
                static_cast<int>(handler.get_up_variable(cycle_node, next_cycle_node))
            );
            at_least_one_right.push_back(
                static_cast<int>(handler.get_right_variable(cycle_node, next_cycle_node))
            );
            at_least_one_left.push_back(
                static_cast<int>(handler.get_left_variable(cycle_node, next_cycle_node))
            );
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
