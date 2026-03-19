#include "domus/orthogonal/shape/shape.hpp"

#include "../../core/domus_debug.hpp"

size_t Shape::direction_to_size_t(Direction direction) const {
    switch (direction) {
    case Direction::LEFT:
        return 0;
    case Direction::RIGHT:
        return 1;
    case Direction::UP:
        return 2;
    case Direction::DOWN:
        return 3;
    default:
        DOMUS_ASSERT(false, "Shape::direction_to_size_t: invalid direction");
        return 4;
    }
}

Direction Shape::size_t_to_direction(size_t direction) const {
    switch (direction) {
    case 0:
        return Direction::LEFT;
    case 1:
        return Direction::RIGHT;
    case 2:
        return Direction::UP;
    case 3:
        return Direction::DOWN;
    default:
        DOMUS_ASSERT(false, "Shape::size_t_to_direction: invalid direction");
        return Direction::INVALID;
    }
}

void Shape::set_direction(size_t node_id_1, size_t node_id_2, const Direction direction) {
    DOMUS_ASSERT(
        !m_shape.contains({node_id_1, node_id_2}),
        "Shape::set_direction: direction already set for this pair"
    );
    m_shape[{node_id_1, node_id_2}] = direction_to_size_t(direction);
}

Direction Shape::get_direction(size_t node_id_1, size_t node_id_2) const {
    DOMUS_ASSERT(
        m_shape.contains({node_id_1, node_id_2}),
        "Shape::get_direction: direction does not exist"
    );
    return size_t_to_direction(m_shape.at({node_id_1, node_id_2}));
}

bool Shape::contains(size_t node_id_1, size_t node_id_2) const {
    return m_shape.contains({node_id_1, node_id_2});
}

bool Shape::is_up(size_t node_id_1, size_t node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::UP;
}

bool Shape::is_down(size_t node_id_1, size_t node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::DOWN;
}

bool Shape::is_right(size_t node_id_1, size_t node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::RIGHT;
}

bool Shape::is_left(size_t node_1_id, size_t node_2_id) const {
    return get_direction(node_1_id, node_2_id) == Direction::LEFT;
}

bool Shape::is_horizontal(size_t node_id_1, size_t node_id_2) const {
    return is_right(node_id_1, node_id_2) || is_left(node_id_1, node_id_2);
}

bool Shape::is_vertical(size_t node_id_1, size_t node_id_2) const {
    return is_up(node_id_1, node_id_2) || is_down(node_id_1, node_id_2);
}

void Shape::remove_direction(size_t node_id_1, size_t node_id_2) {
    DOMUS_ASSERT(
        contains(node_id_1, node_id_2),
        "Shape::remove_direction: direction does not exist for this pair"
    );
    m_shape.erase({node_id_1, node_id_2});
}