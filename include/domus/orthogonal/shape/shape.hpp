#pragma once

#include <string>
#include <unordered_map>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/direction.hpp"

class Shape {
    std::unordered_map<Edge, size_t, edge_hash> m_shape;
    size_t direction_to_size_t(Direction direction) const;
    Direction size_t_to_direction(size_t direction) const;

  public:
    void set_direction(size_t node_id_1, size_t node_id_2, Direction direction);
    Direction get_direction(size_t node_id_1, size_t node_id_2) const;
    bool contains(size_t node_id_1, size_t node_id_2) const;
    bool is_up(size_t node_id_1, size_t node_id_2) const;
    bool is_down(size_t node_id_1, size_t node_id_2) const;
    bool is_right(size_t node_id_1, size_t node_id_2) const;
    bool is_left(size_t node_id_1, size_t node_id_2) const;
    bool is_horizontal(size_t node_id_1, size_t node_id_2) const;
    bool is_vertical(size_t node_id_1, size_t node_id_2) const;
    void remove_direction(size_t node_id_1, size_t node_id_2);
    std::string to_string() const;
    void print() const;
};