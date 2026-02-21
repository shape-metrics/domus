#ifndef MY_SHAPE_HPP
#define MY_SHAPE_HPP

#include <array>
#include <expected>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "domus/core/utils.hpp"

enum class Direction { LEFT, RIGHT, UP, DOWN };

std::string direction_to_string(Direction direction);

std::expected<Direction, std::string> string_to_direction(const std::string& direction);

Direction opposite_direction(Direction direction);

Direction rotate_90_degrees(Direction direction);

bool is_horizontal(Direction direction);

bool is_vertical(Direction direction);

constexpr std::array<Direction, 4> get_all_directions() {
    return {Direction::LEFT, Direction::RIGHT, Direction::UP, Direction::DOWN};
}

class Shape {
    std::unordered_map<std::pair<int, int>, Direction, int_pair_hash> m_shape;

  public:
    std::expected<void, std::string>
    set_direction(int node_id_1, int node_id_2, Direction direction);
    std::optional<Direction> get_direction(int node_id_1, int node_id_2) const;
    bool contains(int node_id_1, int node_id_2) const;
    bool is_up(int node_id_1, int node_id_2) const;
    bool is_down(int node_id_1, int node_id_2) const;
    bool is_right(int node_id_1, int node_id_2) const;
    bool is_left(int i, int j) const;
    bool is_horizontal(int node_id_1, int node_id_2) const;
    bool is_vertical(int node_id_1, int node_id_2) const;
    std::expected<void, std::string> remove_direction(int node_id_1, int node_id_2);
    std::string to_string() const;
    void print() const;
};

#endif