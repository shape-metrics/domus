#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/orthogonal/shape/direction.hpp"

class Graph;

class VariablesHandler {
    int m_next_var = 1; // 0 is reserved for the empty clause
    std::unordered_map<int, std::pair<int, int>> variable_to_edge;
    std::unordered_map<int, Direction> variable_to_direction;
    std::unordered_map<int, bool> variable_to_value;
    IntPair_ToInt_HashMap m_edge_up_variable;
    IntPair_ToInt_HashMap m_edge_down_variable;
    IntPair_ToInt_HashMap m_edge_right_variable;
    IntPair_ToInt_HashMap m_edge_left_variable;
    void add_variable(int i, int j, Direction direction);
    void add_edge_variables(int i, int j);

  public:
    explicit VariablesHandler(const Graph& graph);
    int get_up_variable(int i, int j) const;
    int get_down_variable(int i, int j) const;
    int get_left_variable(int i, int j) const;
    int get_right_variable(int i, int j) const;
    int get_variable(int i, int j, Direction direction) const;
    const std::pair<int, int>& get_edge_of_variable(int variable) const;
    void set_variable_value(int variable, bool value);
    bool get_variable_value(int variable) const;
    Direction get_direction_of_edge(int i, int j) const;
    std::string to_string() const;
    void print() const;
};