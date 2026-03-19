#include "domus/core/color.hpp"

#include "domus_debug.hpp"

std::string color_to_string(const Color color) {
    switch (color) {
    case Color::RED:
        return "red";
    case Color::BLUE:
        return "blue";
    case Color::BLACK:
        return "black";
    case Color::GREEN:
        return "green";
    case Color::RED_SPECIAL:
        return "darkred";
    default:
        DOMUS_ASSERT(false, "color_to_string: invalid color");
        return "Invalid color";
    }
}

Color string_to_color(const std::string_view color) {
    if (color == "red")
        return Color::RED;
    if (color == "blue")
        return Color::BLUE;
    if (color == "green")
        return Color::GREEN;
    if (color == "black")
        return Color::BLACK;
    DOMUS_ASSERT(false, "string_to_color: invalid color string");
    return Color::BLACK;
}
