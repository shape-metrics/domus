#include "orthogonal/shape/clauses_functions.hpp"

void add_constraints_at_most_one_is_true(CnfBuilder& cnf_builder, int var_1,
                                         int var_2, int var_3, int var_4) {
  // at most one is true (at least three are false)
  // for every possible pair, at least one is false
  cnf_builder.add_clause({-var_1, -var_2});
  cnf_builder.add_clause({-var_1, -var_3});
  cnf_builder.add_clause({-var_1, -var_4});
  cnf_builder.add_clause({-var_2, -var_3});
  cnf_builder.add_clause({-var_2, -var_4});
  cnf_builder.add_clause({-var_3, -var_4});
}

void add_constraints_one_direction_per_edge(const Graph& graph,
                                            CnfBuilder& cnf_builder,
                                            const VariablesHandler& handler,
                                            int up, int down, int right,
                                            int left) {
  cnf_builder.add_clause({up, down, right, left});  // at least one is true
  add_constraints_at_most_one_is_true(cnf_builder, up, down, left, right);
}

void add_constraints_one_direction_per_edge(const Graph& graph,
                                            CnfBuilder& cnf_builder,
                                            const VariablesHandler& handler) {
  for (auto& node : graph.get_nodes()) {
    int i = node.get_id();
    for (auto& edge : node.get_edges()) {
      int j = edge.get_to().get_id();
      if (i > j) continue;
      int up = handler.get_up_variable(i, j);
      int down = handler.get_down_variable(i, j);
      int right = handler.get_right_variable(i, j);
      int left = handler.get_left_variable(i, j);
      add_constraints_one_direction_per_edge(graph, cnf_builder, handler, up,
                                             down, right, left);
    }
  }
}

void add_clause_at_least_one_in_direction(CnfBuilder& cnf_builder,
                                          const VariablesHandler& handler,
                                          const GraphNode& node,
                                          Direction direction) {
  std::vector<int> clause;
  int node_id = node.get_id();
  for (auto& edge : node.get_edges()) {
    int neighbor_id = edge.get_to().get_id();
    clause.push_back(handler.get_variable(node_id, neighbor_id, direction));
  }
  cnf_builder.add_clause(clause);
}

void add_one_edge_per_direction_clauses(CnfBuilder& cnf_builder,
                                        const VariablesHandler& handler,
                                        const Direction direction,
                                        const GraphNode& node) {
  int i = node.get_id();
  int degree = node.get_degree();
  if (degree == 4) {
    add_clause_at_least_one_in_direction(cnf_builder, handler, node, direction);
  } else if (degree == 3) {
    std::vector<int> variables;
    for (auto& edge : node.get_edges())
      variables.push_back(
          handler.get_variable(i, edge.get_to().get_id(), direction));
    // at most one is true (at least 2 are false)
    cnf_builder.add_clause({-variables[0], -variables[1]});
    cnf_builder.add_clause({-variables[0], -variables[2]});
    cnf_builder.add_clause({-variables[1], -variables[2]});
  } else if (degree == 2) {
    std::vector<int> clause;
    for (auto& edge : node.get_edges())
      clause.push_back(
          -handler.get_variable(i, edge.get_to().get_id(), direction));
    // at most one is true (at least 1 is false)
    cnf_builder.add_clause(clause);
  } else if (degree != 1) {
    throw std::runtime_error("degree of node is not valid");
  }
}

void add_cycles_constraints(const Graph& graph, CnfBuilder& cnf_builder,
                            const std::vector<Cycle>& cycles,
                            const VariablesHandler& handler) {
  for (auto& cycle : cycles) {
    auto at_least_one_down = std::vector<int>();
    auto at_least_one_up = std::vector<int>();
    auto at_least_one_right = std::vector<int>();
    auto at_least_one_left = std::vector<int>();
    for (int i = 0; i < cycle.size(); i++) {
      at_least_one_down.push_back(
          handler.get_down_variable(cycle[i], cycle[i + 1]));
      at_least_one_up.push_back(
          handler.get_up_variable(cycle[i], cycle[i + 1]));
      at_least_one_right.push_back(
          handler.get_right_variable(cycle[i], cycle[i + 1]));
      at_least_one_left.push_back(
          handler.get_left_variable(cycle[i], cycle[i + 1]));
    }
    cnf_builder.add_clause(at_least_one_down);
    cnf_builder.add_clause(at_least_one_up);
    cnf_builder.add_clause(at_least_one_right);
    cnf_builder.add_clause(at_least_one_left);
  }
}

void add_nodes_constraints(const Graph& graph, CnfBuilder& cnf_builder,
                           const VariablesHandler& handler) {
  for (auto& node : graph.get_nodes()) {
    if (node.get_degree() <= 4) {
      add_one_edge_per_direction_clauses(cnf_builder, handler, Direction::UP,
                                         node);
      add_one_edge_per_direction_clauses(cnf_builder, handler, Direction::DOWN,
                                         node);
      add_one_edge_per_direction_clauses(cnf_builder, handler, Direction::RIGHT,
                                         node);
      add_one_edge_per_direction_clauses(cnf_builder, handler, Direction::LEFT,
                                         node);
    } else {
      add_clause_at_least_one_in_direction(cnf_builder, handler, node,
                                           Direction::UP);
      add_clause_at_least_one_in_direction(cnf_builder, handler, node,
                                           Direction::DOWN);
      add_clause_at_least_one_in_direction(cnf_builder, handler, node,
                                           Direction::RIGHT);
      add_clause_at_least_one_in_direction(cnf_builder, handler, node,
                                           Direction::LEFT);
    }
  }
}