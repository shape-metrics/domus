#include "domus/orthogonal/shape/direction.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::orthogonal::shape {

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
        DOMUS_ASSERT(false, "direction_to_string: invalid direction");
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
    DOMUS_ASSERT(false, "string_to_direction: invalid direction string");
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
        DOMUS_ASSERT(false, "opposite_direction: invalid direction");
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
        DOMUS_ASSERT(false, "rotate_90_degrees: invalid direction");
        return Direction::INVALID;
    }
}

bool is_horizontal(Direction direction) {
    return direction == Direction::LEFT || direction == Direction::RIGHT;
}

bool is_vertical(Direction direction) { return !is_horizontal(direction); }

} // namespace domus::orthogonal::shape