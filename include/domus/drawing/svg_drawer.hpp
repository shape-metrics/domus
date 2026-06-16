#pragma once

#include <expected>
#include <filesystem>
#include <sstream>
#include <string>

#include "domus/core/color.hpp"
#include "domus/drawing/linear_scale.hpp"

namespace domus::drawing {

struct Circle2D;
struct Line2D;
struct Path2D;
struct Polygon2D;
struct Square2D;
struct Point2D;
struct RoundSquare2D;

// TODO usare format non sstream?
class Drawer {
    std::stringstream m_svg;
    double m_width, m_height;
    ScaleLinear m_scale_y;

  public:
    Drawer(double width, double height);
    void add(const std::string_view text, const Point2D& center);
    void add(const Square2D& square, color::ColorRGB color = BLACK_RGB);
    void add(const RoundSquare2D& square, color::ColorRGB color = BLACK_RGB);
    void add(const Circle2D& circle, color::ColorRGB color = BLACK_RGB);
    void add(const Line2D& line, color::ColorRGB color = BLACK_RGB);
    void add(const Polygon2D& polygon, color::ColorRGB color = BLACK_RGB);
    void add(const Path2D& path, color::ColorRGB color = BLACK_RGB);
    void add_and_smooth(Path2D& path, color::ColorRGB color = BLACK_RGB);
    std::expected<void, std::string> save_to_file(std::filesystem::path path);
};

} // namespace domus::drawing