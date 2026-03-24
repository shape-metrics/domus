#pragma once

#include <vector>

class Cnf;
namespace domus::graph {
class Cycle;
class Graph;
} // namespace domus::graph
enum class Direction;

namespace domus::orthogonal::shape {
using Graph = graph::Graph;
class VariablesHandler;

// each edge can only be in one direction
void add_constraints_one_direction_per_edge(
    const Graph& graph, Cnf& cnf_builder, const VariablesHandler& handler
);

// at least one neighbor of node is in the direction
void add_clause_at_least_one_in_direction(
    const Graph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    size_t node_id,
    Direction direction
);

// no two neighbors of node can be in the same direction
void add_one_edge_per_direction_clauses(
    const Graph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    Direction direction,
    size_t node_id
);

void add_nodes_constraints(const Graph& graph, Cnf& cnf_builder, const VariablesHandler& handler);

void add_cycles_constraints(
    const Graph& graph,
    Cnf& cnf_builder,
    const std::vector<graph::Cycle>& cycles,
    const VariablesHandler& handler
);

} // namespace domus::orthogonal::shape