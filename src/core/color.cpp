#include "domus/core/color.hpp"

#include <cassert>

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
        assert(false && "Invalid color");
        return "Invalid color";
    }
}

Color string_to_color(const std::string& color) {
    if (color == "red")
        return Color::RED;
    if (color == "blue")
        return Color::BLUE;
    if (color == "green")
        return Color::GREEN;
    if (color == "black")
        return Color::BLACK;
    assert(false && "Invalid color string");
    return Color::BLACK;
}
