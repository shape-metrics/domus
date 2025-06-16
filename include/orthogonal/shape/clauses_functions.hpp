#ifndef MY_CLAUSES_FUNCTIONS_H
#define MY_CLAUSES_FUNCTIONS_H

#include "core/graph/cycle.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/shape/variables_handler.hpp"
#include "sat/cnf_builder.hpp"

// each edge can only be in one direction
void add_constraints_one_direction_per_edge(const Graph& graph,
                                            CnfBuilder& cnf_builder,
                                            const VariablesHandler& handler);

// at least one neighbor of node is in the direction
void add_clause_at_least_one_in_direction(CnfBuilder& cnf_builder,
                                          const VariablesHandler& handler,
                                          const GraphNode& node,
                                          Direction direction);

// no two neighbors of node can be in the same direction
void add_one_edge_per_direction_clauses(CnfBuilder& cnf_builder,
                                        const VariablesHandler& handler,
                                        const Direction direction,
                                        const GraphNode& node);

void add_nodes_constraints(const Graph& graph, CnfBuilder& cnf_builder,
                           const VariablesHandler& handler);

void add_cycles_constraints(const Graph& graph, CnfBuilder& cnf_builder,
                            const std::vector<Cycle>& cycles,
                            const VariablesHandler& handler);

#endif