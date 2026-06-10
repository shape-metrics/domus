#pragma once

#include <expected>
#include <filesystem>
#include <sstream>
#include <string>

#include "domus/drawing/linear_scale.hpp"

namespace domus::drawing {

class Circle2D;
class Line2D;
struct Path2D;
class Polygon2D;
class Square2D;
class Point2D;
class RoundSquare2D;

// TODO usare format non sstream?
class SvgDrawer {
    std::stringstream m_svg;
    int m_width, m_height;
    ScaleLinear m_scale_y;

  public:
    SvgDrawer(int width, int height);
    void add(const std::string_view text, const Point2D& center);
    void add(const Square2D& square, const std::string_view color = "black");
    void add(const RoundSquare2D& square, const std::string_view color = "black");
    void add(const Circle2D& circle, const std::string_view color = "black");
    void add(const Line2D& line, const std::string_view color = "black");
    void add(const Polygon2D& polygon, const std::string_view color = "black");
    void add(const Path2D& path, const std::string_view color);
    void add_and_smooth(Path2D& path, const std::string_view color = "black");
    std::expected<void, std::string> save_to_file(std::filesystem::path path);
};

} // namespace domus::drawing