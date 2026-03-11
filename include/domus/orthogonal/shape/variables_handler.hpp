#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/orthogonal/shape/direction.hpp"

class Graph;

class VariablesHandler {
    size_t m_next_var = 1; // 0 is reserved for the empty clause
    std::unordered_map<size_t, std::pair<size_t, size_t>> variable_to_edge;
    std::unordered_map<size_t, Direction> variable_to_direction;
    std::unordered_map<size_t, bool> variable_to_value;
    IntPair_ToInt_HashMap m_edge_up_variable;
    IntPair_ToInt_HashMap m_edge_down_variable;
    IntPair_ToInt_HashMap m_edge_right_variable;
    IntPair_ToInt_HashMap m_edge_left_variable;
    void add_variable(size_t i, size_t j, Direction direction);
    void add_edge_variables(size_t i, size_t j);

  public:
    explicit VariablesHandler(const Graph& graph);
    size_t get_up_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_down_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_left_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_right_variable(size_t node_id_1, size_t node_id_2) const;
    size_t get_variable(size_t node_id_1, size_t node_id_2, Direction direction) const;
    const std::pair<size_t, size_t>& get_edge_of_variable(size_t variable) const;
    void set_variable_value(size_t variable, bool value);
    bool get_variable_value(size_t variable) const;
    Direction get_direction_of_edge(size_t node_id_1, size_t node_id_2) const;
    std::string to_string() const;
    void print() const;
};