#include "domus/drawing/svg_drawer.hpp"

#include <fstream>
#include <stddef.h>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/drawing/polygon.hpp"

namespace domus::drawing {
using color::ColorRGB;

Drawer::Drawer(double width, double height)
    : m_width(width), m_height(height), m_scale_y(0, height, height, 0) {
    m_svg << "<svg height=\"" << m_height << "\" width=\"" << m_width << "\" ";
    m_svg << "xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    m_svg << "<rect width=\"" << m_width << "\" height=\"" << m_height << "\" ";
    m_svg << "x=\"0\" y=\"0\" fill=\"white\" />";
}

void Drawer::add(const std::string_view text, const Point2D& center) {
    m_svg << "<text x=\"" << center.x << "\" y=\"" << m_scale_y.map(center.y) << "\" ";
    m_svg << "font-family=\"Verdana\" font-size=\"18\" fill=\"white\" ";
    m_svg << "text-anchor=\"middle\" dominant-baseline=\"central\">" << text << "</text>\n";
}

void Drawer::add(const Square2D& square, ColorRGB color) {
    const double x = square.center.x - square.side / 2;
    const double y = square.center.y + square.side / 2;
    m_svg << "<rect x=\"" << x << "\" y=\"" << m_scale_y.map(y) << "\" ";
    m_svg << "width=\"" << square.side << "\" height=\"" << square.side << "\" fill=\""
          << color_to_string(color) << "\" />" << std::endl;
}

void Drawer::add(const RoundSquare2D& square, ColorRGB color) {
    const double x = square.center.x - square.side / 2;
    const double y = square.center.y + square.side / 2;
    m_svg << "<rect x=\"" << x << "\" y=\"" << m_scale_y.map(y) << "\" ";
    m_svg << "rx=\"" << square.corner_radious << "\" ";
    m_svg << "width=\"" << square.side << "\" height=\"" << square.side << "\" fill=\""
          << color_to_string(color) << "\" />" << std::endl;
}

void Drawer::add(const Circle2D& circle, ColorRGB color) {
    m_svg << "<circle cx=\"" << circle.center.x << "\" cy=\"" << m_scale_y.map(circle.center.y)
          << "\" ";
    m_svg << "r=\"" << circle.radius << "\" fill=\"" << color_to_string(color) << "\" />\n";
}

void Drawer::add(const Line2D& line, ColorRGB color) {
    m_svg << "<line x1=\"" << line.m_p1.x << "\" y1=\"" << m_scale_y.map(line.m_p1.y) << "\" ";
    m_svg << "x2=\"" << line.m_p2.x << "\" y2=\"" << m_scale_y.map(line.m_p2.y) << "\" ";
    m_svg << "style=\"stroke:" << color_to_string(color) << ";stroke-width:2\" />" << std::endl;
}

void Drawer::add(const Polygon2D& polygon, ColorRGB color) {
    m_svg << "<polygon points=\"";
    for (const Point2D& point : polygon.points)
        m_svg << point.x << "," << m_scale_y.map(point.y) << " ";
    m_svg << "\" style=\"fill:white;stroke:" << color_to_string(color) << ";stroke-width:2\" />"
          << std::endl;
}

void Drawer::add(const Path2D& path, ColorRGB color) {
    m_svg << "<path d=\"";
    for (size_t i = 0; i < path.points.size(); i++) {
        if (i == 0)
            m_svg << "M" << path.points[i].x << "," << m_scale_y.map(path.points[i].y) << " ";
        else
            m_svg << "L" << path.points[i].x << "," << m_scale_y.map(path.points[i].y) << " ";
    }
    m_svg << "\" style=\"fill:none;stroke:" << color_to_string(color) << ";stroke-width:1\" />"
          << std::endl;
}

void Drawer::add_and_smooth(Path2D& path, ColorRGB color) {
    m_svg << "<path d=\"";
    for (size_t i = 0; i < path.points.size(); i++) {
        if (i == 0)
            m_svg << "M" << path.points[i].x << "," << m_scale_y.map(path.points[i].y) << " ";
        else
            m_svg << "T" << path.points[i].x << "," << m_scale_y.map(path.points[i].y) << " ";
    }
    m_svg << "\" style=\"fill:none;stroke:" << color_to_string(color) << ";stroke-width:1\" />"
          << std::endl;
}

std::expected<void, std::string> Drawer::save_to_file(std::filesystem::path path) {
    std::ofstream svg_file(path);
    if (!svg_file.is_open()) {
        return std::unexpected(
            std::format("SvgDrawer::save_to_file: could not open file {}", path.string())
        );
    }
    svg_file << m_svg.str();
    svg_file << "</svg>" << std::endl;
    return {};
}

} // namespace domus::drawing