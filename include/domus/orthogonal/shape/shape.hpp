#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domus/orthogonal/shape/direction.hpp"

class Graph;

class Shape {
    std::vector<std::optional<Direction>> m_edge_id_to_direction;

  public:
    bool contains(size_t edge_id) const;
    Direction get_direction(size_t edge_id) const;
    Direction get_direction(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;

    bool is_up(size_t edge_id) const;
    bool is_up(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_down(size_t edge_id) const;
    bool is_down(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_right(size_t edge_id) const;
    bool is_right(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_left(size_t edge_id) const;
    bool is_left(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;

    bool is_horizontal(size_t edge_id) const;
    bool is_vertical(size_t edge_id) const;
    bool are_perpendicular(size_t edge_id_1, size_t edge_id_2) const;
    bool are_parallel(size_t edge_id_1, size_t edge_id_2) const;

    void set_direction(size_t edge_id, Direction direction);
    void set_direction(
        const Graph& graph, size_t edge_id, size_t from_id, size_t to_id, Direction direction
    );
    void remove_direction(size_t edge_id);
    void update_direction(size_t edge_id, Direction direction);

    std::string to_string() const;
    void print() const;
};

bool is_shape_valid(const Graph& graph, const Shape& shape);