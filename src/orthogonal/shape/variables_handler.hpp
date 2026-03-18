#pragma once

#include <string>
#include <unordered_map>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/direction.hpp"

class Graph;

class VariablesHandler {
    size_t m_next_var = 1; // 0 is reserved for the empty clause
    std::vector<Edge> variable_to_edge;
    std::vector<Direction> variable_to_direction;
    std::vector<int> variable_to_value;
    std::unordered_map<Edge, size_t, edge_hash> m_edge_up_variable;
    std::unordered_map<Edge, size_t, edge_hash> m_edge_down_variable;
    std::unordered_map<Edge, size_t, edge_hash> m_edge_right_variable;
    std::unordered_map<Edge, size_t, edge_hash> m_edge_left_variable;
    void add_variable(size_t i, size_t j, Direction direction);
    void add_edge_variables(size_t i, size_t j);

  public:
    explicit VariablesHandler(const Graph& graph);
    size_t get_up_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_down_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_left_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_right_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_variable(size_t node_id_1, size_t node_id_2, Direction direction) const;
    const Edge& get_edge_of_variable(size_t variable) const;
    void set_variable_value(size_t variable, bool value);
    bool get_variable_value(size_t variable) const;
    Direction get_direction_of_edge(size_t node_id_1, size_t node_id_2) const;
    std::string to_string() const;
    void print() const;
};