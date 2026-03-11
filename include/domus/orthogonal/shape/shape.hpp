#pragma once

#include <optional>
#include <string>

#include "domus/core/containers.hpp"
#include "domus/orthogonal/shape/direction.hpp"

class Shape {
    IntPair_ToInt_HashMap m_shape;
    int direction_to_int(Direction direction) const;
    Direction int_to_direction(int direction) const;

  public:
    void set_direction(int node_id_1, int node_id_2, Direction direction);
    std::optional<Direction> get_direction(int node_id_1, int node_id_2) const;
    bool contains(int node_id_1, int node_id_2) const;
    bool is_up(int node_id_1, int node_id_2) const;
    bool is_down(int node_id_1, int node_id_2) const;
    bool is_right(int node_id_1, int node_id_2) const;
    bool is_left(int i, int j) const;
    bool is_horizontal(int node_id_1, int node_id_2) const;
    bool is_vertical(int node_id_1, int node_id_2) const;
    void remove_direction(int node_id_1, int node_id_2);
    std::string to_string() const;
    void print() const;
};