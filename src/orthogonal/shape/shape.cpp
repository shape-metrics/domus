#include "domus/orthogonal/shape/shape.hpp"

#include "domus/core/graph/graph.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::orthogonal::shape {
using namespace graph;

void Shape::set_direction(size_t edge_id, const Direction direction) {
    DOMUS_ASSERT(!contains(edge_id), "Shape::set_direction: direction already set");
    while (m_edge_id_to_direction.size() <= edge_id)
        m_edge_id_to_direction.push_back(std::nullopt);
    m_edge_id_to_direction[edge_id] = direction;
}

Direction Shape::get_direction(size_t edge_id) const {
    DOMUS_ASSERT(contains(edge_id), "Shape::get_direction: direction does not exist");
    return *m_edge_id_to_direction.at(edge_id);
}

Direction
Shape::get_direction(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const {
    auto [_from_id, _to_id] = graph.get_edge(edge_id);
    if (_from_id == from_id) {
        DOMUS_ASSERT(
            to_id == _to_id,
            "Shape::get_direction: from_id and to_id do not match with edge"
        );
        return get_direction(edge_id);
    } else {
        DOMUS_ASSERT(
            to_id == _from_id,
            "Shape::get_direction: from_id and to_id do not match with edge"
        );
        DOMUS_ASSERT(
            from_id == _to_id,
            "Shape::get_direction: from_id and to_id do not match with edge"
        );
        return opposite_direction(get_direction(edge_id));
    }
}

bool Shape::contains(size_t edge_id) const {
    if (m_edge_id_to_direction.size() <= edge_id)
        return false;
    return m_edge_id_to_direction[edge_id].has_value();
}

bool Shape::is_up(size_t edge_id) const { return get_direction(edge_id) == Direction::UP; }

bool Shape::is_down(size_t edge_id) const { return get_direction(edge_id) == Direction::DOWN; }

bool Shape::is_right(size_t edge_id) const { return get_direction(edge_id) == Direction::RIGHT; }

bool Shape::is_left(size_t edge_id) const { return get_direction(edge_id) == Direction::LEFT; }

bool Shape::is_horizontal(size_t edge_id) const {
    Direction d = get_direction(edge_id);
    return d == Direction::RIGHT || d == Direction::LEFT;
}

bool Shape::is_vertical(size_t edge_id) const {
    Direction d = get_direction(edge_id);
    return d == Direction::UP || d == Direction::DOWN;
}

void Shape::remove_direction(size_t edge_id) {
    DOMUS_ASSERT(
        contains(edge_id),
        "Shape::remove_direction: direction does not exist for this edge"
    );
    m_edge_id_to_direction[edge_id] = std::nullopt;
}

bool Shape::is_up(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const {
    return get_direction(graph, edge_id, from_id, to_id) == Direction::UP;
}

bool Shape::is_down(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const {
    return get_direction(graph, edge_id, from_id, to_id) == Direction::DOWN;
}

bool Shape::is_right(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const {
    return get_direction(graph, edge_id, from_id, to_id) == Direction::RIGHT;
}

bool Shape::is_left(const Graph& graph, size_t edge_id, size_t from_id, size_t to_id) const {
    return get_direction(graph, edge_id, from_id, to_id) == Direction::LEFT;
}

void Shape::set_direction(
    const Graph& graph, size_t edge_id, size_t from_id, size_t to_id, Direction direction
) {
    auto [_from_id, _to_id] = graph.get_edge(edge_id);
    if (_from_id == from_id) {
        DOMUS_ASSERT(
            to_id == _to_id,
            "Shape::set_direction: from_id and to_id do not match with edge"
        );
        set_direction(edge_id, direction);
    } else {
        DOMUS_ASSERT(
            to_id == _from_id,
            "Shape::set_direction: from_id and to_id do not match with edge"
        );
        DOMUS_ASSERT(
            from_id == _to_id,
            "Shape::set_direction: from_id and to_id do not match with edge"
        );
        set_direction(edge_id, opposite_direction(direction));
    }
}

bool Shape::are_perpendicular(size_t edge_id_1, size_t edge_id_2) const {
    DOMUS_ASSERT(contains(edge_id_1), "Shape::are_perpendicular: edge_id_1 not in shape");
    DOMUS_ASSERT(contains(edge_id_2), "Shape::are_perpendicular: edge_id_2 not in shape");
    return is_horizontal(edge_id_1) != is_horizontal(edge_id_2);
}

bool Shape::are_parallel(size_t edge_id_1, size_t edge_id_2) const {
    DOMUS_ASSERT(contains(edge_id_1), "Shape::are_parallel: edge_id_1 not in shape");
    DOMUS_ASSERT(contains(edge_id_2), "Shape::are_parallel: edge_id_2 not in shape");
    return is_horizontal(edge_id_1) == is_horizontal(edge_id_2);
}

bool is_shape_valid(const Graph& graph, const Shape& shape) {
    for (const size_t node_id : graph.get_nodes_ids()) {
        for (const EdgeIter edge : graph.get_out_edges(node_id)) {
            if (!shape.contains(edge.id))
                return false;
            if (shape.get_direction(graph, edge.id, node_id, edge.neighbor_id) !=
                opposite_direction(shape.get_direction(graph, edge.id, edge.neighbor_id, node_id)))
                return false;
        }
    }
    return true;
}

} // namespace domus::orthogonal::shape