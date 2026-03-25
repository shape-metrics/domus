#pragma once

#include <vector>

namespace domus::sat::cnf {
class Cnf;
}

namespace domus::graph {
class Cycle;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal::shape {
enum class Direction;
class VariablesHandler;

// each edge can only be in one direction
void add_constraints_one_direction_per_edge(
    const graph::Graph& graph, sat::cnf::Cnf& cnf_builder, const VariablesHandler& handler
);

// at least one neighbor of node is in the direction
void add_clause_at_least_one_in_direction(
    const graph::Graph& graph,
    sat::cnf::Cnf& cnf_builder,
    const VariablesHandler& handler,
    size_t node_id,
    Direction direction
);

// no two neighbors of node can be in the same direction
void add_one_edge_per_direction_clauses(
    const graph::Graph& graph,
    sat::cnf::Cnf& cnf_builder,
    const VariablesHandler& handler,
    Direction direction,
    size_t node_id
);

void add_nodes_constraints(
    const graph::Graph& graph, sat::cnf::Cnf& cnf_builder, const VariablesHandler& handler
);

void add_cycles_constraints(
    const graph::Graph& graph,
    sat::cnf::Cnf& cnf_builder,
    const std::vector<graph::Cycle>& cycles,
    const VariablesHandler& handler
);

} // namespace domus::orthogonal::shape