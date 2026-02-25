#include "domus/orthogonal/shape/shape.hpp"

#include <cassert>

using namespace std;

std::string direction_to_string(const Direction direction) {
    switch (direction) {
    case Direction::LEFT:
        return "left";
    case Direction::RIGHT:
        return "right";
    case Direction::UP:
        return "up";
    case Direction::DOWN:
        return "down";
    default:
        assert(false && "Invalid direction");
        return "Invalid direction";
    }
}

Direction string_to_direction(const std::string& direction) {
    if (direction == "left")
        return Direction::LEFT;
    if (direction == "right")
        return Direction::RIGHT;
    if (direction == "up")
        return Direction::UP;
    if (direction == "down")
        return Direction::DOWN;
    assert(false && "Error in string_to_direction: invalid direction string");
    return Direction::INVALID;
}

Direction opposite_direction(const Direction direction) {
    switch (direction) {
    case Direction::LEFT:
        return Direction::RIGHT;
    case Direction::RIGHT:
        return Direction::LEFT;
    case Direction::UP:
        return Direction::DOWN;
    case Direction::DOWN:
        return Direction::UP;
    default:
        assert(false && "Invalid direction");
        return Direction::INVALID;
    }
}

Direction rotate_90_degrees(const Direction direction) {
    switch (direction) {
    case Direction::LEFT:
        return Direction::UP;
    case Direction::RIGHT:
        return Direction::DOWN;
    case Direction::UP:
        return Direction::RIGHT;
    case Direction::DOWN:
        return Direction::LEFT;
    default:
        assert(false && "Invalid direction");
        return Direction::INVALID;
    }
}

bool is_horizontal(Direction direction) {
    return direction == Direction::LEFT || direction == Direction::RIGHT;
}

bool is_vertical(Direction direction) { return !is_horizontal(direction); }

int Shape::direction_to_int(Direction direction) const {
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
        assert(false && "Invalid direction");
        return -1;
    }
}

Direction Shape::int_to_direction(int direction) const {
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
        assert(false && "Invalid direction");
        return Direction::INVALID;
    }
}

void Shape::set_direction(const int node_id_1, const int node_id_2, const Direction direction) {
    assert(
        !m_shape.has(node_id_1, node_id_2) &&
        "Error in Shape::set_direction: direction already set for this pair"
    );
    m_shape.add(node_id_1, node_id_2, direction_to_int(direction));
}

optional<Direction> Shape::get_direction(const int node_id_1, const int node_id_2) const {
    if (!m_shape.has(node_id_1, node_id_2))
        return std::nullopt;
    return int_to_direction(m_shape.get(node_id_1, node_id_2));
}

bool Shape::contains(const int node_id_1, const int node_id_2) const {
    return m_shape.has(node_id_1, node_id_2);
}

bool Shape::is_up(const int node_id_1, const int node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::UP;
}

bool Shape::is_down(const int node_id_1, const int node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::DOWN;
}

bool Shape::is_right(const int node_id_1, const int node_id_2) const {
    return get_direction(node_id_1, node_id_2) == Direction::RIGHT;
}

bool Shape::is_left(const int i, const int j) const {
    return get_direction(i, j) == Direction::LEFT;
}

bool Shape::is_horizontal(const int node_id_1, const int node_id_2) const {
    return is_right(node_id_1, node_id_2) || is_left(node_id_1, node_id_2);
}

bool Shape::is_vertical(const int node_id_1, const int node_id_2) const {
    return is_up(node_id_1, node_id_2) || is_down(node_id_1, node_id_2);
}

void Shape::remove_direction(const int node_id_1, const int node_id_2) {
    assert(
        contains(node_id_1, node_id_2) &&
        "Error in Shape::remove_direction: direction does not exist for this pair"
    );
    m_shape.erase(node_id_1, node_id_2);
}