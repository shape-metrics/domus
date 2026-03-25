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

// TODO usare format non sstream?
class SvgDrawer {
    std::stringstream m_svg;
    int m_width, m_height;
    ScaleLinear m_scale_y;

  public:
    SvgDrawer(int width, int height);
    void add(Square2D& square, double corner_radious = 0);
    void add(Circle2D& circle);
    void add(Line2D& line, const std::string_view color = "black");
    void add(Polygon2D& polygon, const std::string_view color = "black");
    void add(const Path2D& path, const std::string_view color);
    void add_and_smooth(Path2D& path, const std::string_view color = "black");
    std::expected<void, std::string> save_to_file(std::filesystem::path path);
};

} // namespace domus::drawing