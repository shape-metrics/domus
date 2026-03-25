#pragma once

#include <string>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/direction.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::orthogonal::shape {
class VariablesHandler {
    size_t m_next_var = 1; // 0 is reserved for the empty clause
    std::vector<size_t> m_variable_to_edge_id;
    std::vector<Direction> m_variable_to_direction;
    std::vector<int> m_variable_to_value;
    std::vector<std::optional<size_t>> m_edge_up_variable;
    std::vector<std::optional<size_t>> m_edge_down_variable;
    std::vector<std::optional<size_t>> m_edge_right_variable;
    std::vector<std::optional<size_t>> m_edge_left_variable;
    void add_variable(size_t edge_id, Direction direction);
    void add_edge_variables(size_t edge_id);

  public:
    VariablesHandler(const graph::Graph& graph);
    size_t get_up_variable(size_t edge_id) const;
    size_t get_down_variable(size_t edge_id) const;
    size_t get_left_variable(size_t edge_id) const;
    size_t get_right_variable(size_t edge_id) const;
    size_t get_variable(size_t edge_id, Direction direction) const;
    size_t get_edge_id_of_variable(size_t variable) const;
    void set_variable_value(size_t variable, bool value);
    bool get_variable_value(size_t variable) const;
    Direction get_direction_of_edge(size_t edge_id) const;
    std::string to_string() const;
    void print() const;
};

} // namespace domus::orthogonal::shape