#ifndef MY_CLAUSES_FUNCTIONS_H
#define MY_CLAUSES_FUNCTIONS_H

#include <vector>

class Cnf;
class Cycle;
class UndirectedGraph;
class VariablesHandler;
enum class Direction;

// each edge can only be in one direction
void add_constraints_one_direction_per_edge(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler);

// at least one neighbor of node is in the direction
void add_clause_at_least_one_in_direction(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    int node_id,
    Direction direction);

// no two neighbors of node can be in the same direction
void add_one_edge_per_direction_clauses(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler,
    Direction direction,
    int node_id);

void add_nodes_constraints(
    const UndirectedGraph& graph,
    Cnf& cnf_builder,
    const VariablesHandler& handler);

void add_cycles_constraints(
    Cnf& cnf_builder,
    const std::vector<Cycle>& cycles,
    const VariablesHandler& handler);

#endif