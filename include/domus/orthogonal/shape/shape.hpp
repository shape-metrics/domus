#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/direction.hpp"

namespace domus::graph {
class Graph;
class Cycle;
class Attributes;
} // namespace domus::graph

namespace domus::orthogonal::shape {

enum class NodeType {
    VERTEX, // Color::BLACK
    CORNER,
    MINI_CORNER_1, // Color::BLUE
    MINI_CORNER_2, // Color::green
    MINI_CORNER_3, // DARK_GREEN
    MINI_CORNER_4  // DARK BLUE
};

class Shape {
    std::vector<std::optional<Direction>> m_edge_id_to_direction;

  public:
    bool contains(size_t edge_id) const;
    Direction get_direction(size_t edge_id) const;
    Direction
    get_direction(const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;

    bool is_up(size_t edge_id) const;
    bool is_up(const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_down(size_t edge_id) const;
    bool is_down(const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_right(size_t edge_id) const;
    bool is_right(const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;
    bool is_left(size_t edge_id) const;
    bool is_left(const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const;

    bool is_horizontal(size_t edge_id) const;
    bool is_vertical(size_t edge_id) const;
    bool are_perpendicular(size_t edge_id_1, size_t edge_id_2) const;
    bool are_parallel(size_t edge_id_1, size_t edge_id_2) const;

    void set_direction(size_t edge_id, Direction direction);
    void set_direction(
        const graph::Graph& graph, size_t edge_id, size_t from_id, size_t to_id, Direction direction
    );
    void remove_direction(size_t edge_id);
    void update_direction(size_t edge_id, Direction direction);

    std::string to_string() const;
    void print() const;
};

Shape build_shape(
    graph::Graph& graph,
    graph::utilities::NodesLabels<NodeType>& nodes_types,
    std::vector<graph::Cycle>& cycles,
    bool randomize = false
);

bool is_shape_valid(const graph::Graph& graph, const Shape& shape);

} // namespace domus::orthogonal::shape