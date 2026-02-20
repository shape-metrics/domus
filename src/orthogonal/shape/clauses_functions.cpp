#include "domus/orthogonal/shape/clauses_functions.hpp"

#include <cassert>
#include <stddef.h>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/shape/shape.hpp"
#include "domus/orthogonal/shape/variables_handler.hpp"
#include "domus/sat/cnf.hpp"

using namespace std;

void add_constraints_at_most_one_is_true(
    Cnf& cnf_builder, const int var_1, const int var_2, const int var_3, const int var_4
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
    const UndirectedGraph& graph, Cnf& cnf_builder, const VariablesHandler& handler
) {
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (node_id > neighbor_id)
                continue;
            const int up = handler.get_up_variable(node_id, neighbor_id);
            const int down = handler.get_down_variable(node_id, neighbor_id);
            const int right = handler.get_right_variable(node_id, neighbor_id);
            const int left = handler.get_left_variable(node_id, neighbor_id);
            add_constraints_one_direction_per_edge(cnf_builder, up, down, right, left);
        }
    }
}

void add_clause_at_least_one_in_direction(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    int node_id,
    Direction direction
) {
    vector<int> clause;
    for (int neighbor_id : graph.get_neighbors_of_node(node_id))
        clause.push_back(handler.get_variable(node_id, neighbor_id, direction));
    cnf_builder.add_clause(clause);
}

void add_one_edge_per_direction_clauses(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    const Direction direction,
    int node_id
) {
    size_t degree = graph.get_degree_of_node(node_id);
    if (degree == 4) {
        add_clause_at_least_one_in_direction(graph, cnf_builder, handler, node_id, direction);
    } else if (degree == 3) {
        vector<int> variables;
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            variables.push_back(handler.get_variable(node_id, neighbor_id, direction));
        // at most one is true (at least 2 are false)
        cnf_builder.add_clause({-variables[0], -variables[1]});
        cnf_builder.add_clause({-variables[0], -variables[2]});
        cnf_builder.add_clause({-variables[1], -variables[2]});
    } else if (degree == 2) {
        vector<int> clause;
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            clause.push_back(-handler.get_variable(node_id, neighbor_id, direction));
        // at most one is true (at least 1 is false)
        cnf_builder.add_clause(clause);
    } else if (degree != 1) {
        assert(false); // degree of node is not valid
    }
}

void add_cycles_constraints(
    Cnf& cnf_builder, const vector<Cycle>& cycles, const VariablesHandler& handler
) {
    for (const Cycle& cycle : cycles) {
        vector<int> at_least_one_down{};
        vector<int> at_least_one_up{};
        vector<int> at_least_one_right{};
        vector<int> at_least_one_left{};
        for (const int cycle_node : cycle) {
            const int next_cycle_node = cycle.next_of_node(cycle_node);
            at_least_one_down.push_back(handler.get_down_variable(cycle_node, next_cycle_node));
            at_least_one_up.push_back(handler.get_up_variable(cycle_node, next_cycle_node));
            at_least_one_right.push_back(handler.get_right_variable(cycle_node, next_cycle_node));
            at_least_one_left.push_back(handler.get_left_variable(cycle_node, next_cycle_node));
        }
        cnf_builder.add_clause(at_least_one_down);
        cnf_builder.add_clause(at_least_one_up);
        cnf_builder.add_clause(at_least_one_right);
        cnf_builder.add_clause(at_least_one_left);
    }
}

void add_nodes_constraints(
    const UndirectedGraph& graph, Cnf& cnf_builder, const VariablesHandler& handler
) {
    for (int node_id : graph.get_nodes_ids()) {
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
    }
}
