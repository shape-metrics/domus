#include "domus/orthogonal/shape/shape.hpp"

#include <expected>
#include <iostream>
#include <stdexcept>

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
        throw std::invalid_argument("Invalid direction");
    }
}

expected<Direction, string> string_to_direction(const std::string& direction) {
    if (direction == "left")
        return Direction::LEFT;
    if (direction == "right")
        return Direction::RIGHT;
    if (direction == "up")
        return Direction::UP;
    if (direction == "down")
        return Direction::DOWN;
    string error_msg = "Error in string_to_direction: invalid direction string";
    error_msg += direction;
    error_msg += "\n";
    return std::unexpected(error_msg);
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
        throw std::invalid_argument("Invalid direction");
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
        throw std::invalid_argument("Invalid direction");
    }
}

bool is_horizontal(Direction direction) {
    return direction == Direction::LEFT || direction == Direction::RIGHT;
}

bool is_vertical(Direction direction) { return !is_horizontal(direction); }

expected<void, string>
Shape::set_direction(const int node_id_1, const int node_id_2, const Direction direction) {
    if (m_shape.contains(std::make_pair(node_id_1, node_id_2))) {
        const std::string error_msg =
            "Error in Shape::set_direction: direction already set for this pair: (" +
            std::to_string(node_id_1) + ", " + std::to_string(node_id_2) + ") -> " +
            direction_to_string(m_shape.at(std::make_pair(node_id_1, node_id_2))) + " vs " +
            direction_to_string(direction);
        return std::unexpected(error_msg);
    }
    m_shape[std::make_pair(node_id_1, node_id_2)] = direction;
    return {};
}

optional<Direction> Shape::get_direction(const int node_id_1, const int node_id_2) const {
    if (!m_shape.contains({node_id_1, node_id_2}))
        return std::nullopt;
    return m_shape.at(make_pair(node_id_1, node_id_2));
}

bool Shape::contains(const int node_id_1, const int node_id_2) const {
    return m_shape.contains(make_pair(node_id_1, node_id_2));
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

expected<void, string> Shape::remove_direction(const int node_id_1, const int node_id_2) {
    if (!contains(node_id_1, node_id_2)) {
        string error_msg =
            "Error in Shape::remove_direction: direction does not exist for this pair";
        error_msg += "(" + std::to_string(node_id_1) + ", " + std::to_string(node_id_2) + ")";
        error_msg += "\n";
        return std::unexpected(error_msg);
    }
    m_shape.erase(std::make_pair(node_id_1, node_id_2));
    return {};
}

std::string Shape::to_string() const {
    std::string result = "Shape:\n";
    for (auto& [pair, direction] : m_shape) {
        if (direction == Direction::LEFT)
            continue;
        if (direction == Direction::DOWN)
            continue;
        result +=
            ("(" + std::to_string(pair.first) + " -> " + std::to_string(pair.second) +
             "): " + direction_to_string(direction) + "\n");
    }
    return result;
}

void Shape::print() const { std::cout << to_string() << std::endl; }