#pragma once

#include <array>
#include <string>

namespace domus::orthogonal::shape {

enum class Direction { LEFT, RIGHT, UP, DOWN, INVALID };

std::string direction_to_string(Direction direction);

Direction string_to_direction(const std::string& direction);

Direction opposite_direction(Direction direction);

Direction rotate_90_degrees(Direction direction);

bool is_horizontal(Direction direction);

bool is_vertical(Direction direction);

constexpr std::array<Direction, 4> get_all_directions() {
    return {Direction::LEFT, Direction::RIGHT, Direction::UP, Direction::DOWN};
}

} // namespace domus::orthogonal::shape