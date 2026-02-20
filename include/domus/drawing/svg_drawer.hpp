#ifndef MY_SVG_DRAWER_H
#define MY_SVG_DRAWER_H

#include <expected>
#include <filesystem>
#include <sstream>
#include <string>

#include "domus/drawing/linear_scale.hpp"

class Circle2D;
class Line2D;
class Path2D;
class Polygon2D;
class Square2D;

class SvgDrawer {
    std::stringstream m_svg;
    int m_width, m_height;
    ScaleLinear m_scale_y;

  public:
    SvgDrawer(int width, int height);
    void add(Square2D& square, int corner_radious = 0);
    void add(Circle2D& circle);
    void add(Line2D& line, std::string color = "black");
    void add(Polygon2D& polygon, std::string color = "black");
    void add(const Path2D& path, std::string color);
    void add_and_smooth(Path2D& path, std::string color = "black");
    std::expected<void, std::string> save_to_file(std::filesystem::path path);
};

#endif