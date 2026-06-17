#include "domus/drawing/drawer.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <stddef.h>
#include <vector>

namespace domus::drawing {

Drawer::Drawer(double width, double height)
    : m_width(width), m_height(height), m_scale_y(0, height, height, 0) {}

void Drawer::add(const Text2D& text) { m_elements.push_back(text); }

void Drawer::add(const Square2D& square) { m_elements.push_back(square); }

void Drawer::add(const RoundSquare2D& square) { m_elements.push_back(square); }

void Drawer::add(const Circle2D& circle) { m_elements.push_back(circle); }

void Drawer::add(const Line2D& line) { m_elements.push_back(line); }

void Drawer::add(const Polygon2D& polygon) { m_elements.push_back(polygon); }

void Drawer::add(const Path2D& path) { m_elements.push_back(path); }

void Drawer::add(const SmoothPath2D& path) { m_elements.push_back(path); }

void Drawer::render_to_stream(std::ostream& os) const {
    os << "<svg height=\"" << m_height << "\" width=\"" << m_width << "\" ";
    os << "xmlns=\"http://www.w3.org/2000/svg\">\n";
    os << "<rect width=\"" << m_width << "\" height=\"" << m_height << "\" ";
    os << "x=\"0\" y=\"0\" fill=\"white\" />\n";

    for (const auto& element : m_elements) {
        std::visit(
            [&](const auto& element) {
                using T = std::decay_t<decltype(element)>;
                if constexpr (std::is_same_v<T, Text2D>) {
                    os << "<text x=\"" << element.center.x << "\" y=\""
                       << m_scale_y.map(element.center.y) << "\" ";
                    os << "font-family=\"Verdana\" font-size=\"18\" fill=\"white\" ";
                    os << "text-anchor=\"middle\" dominant-baseline=\"central\">" << element.text
                       << "</text>\n";
                } else if constexpr (std::is_same_v<T, Square2D>) {
                    const double x = element.center.x - element.side / 2;
                    const double y = element.center.y + element.side / 2;
                    os << "<rect x=\"" << x << "\" y=\"" << m_scale_y.map(y) << "\" ";
                    os << "width=\"" << element.side << "\" height=\"" << element.side
                       << "\" fill=\"" << color_to_string(element.color) << "\" />\n";
                } else if constexpr (std::is_same_v<T, RoundSquare2D>) {
                    const double x = element.center.x - element.side / 2;
                    const double y = element.center.y + element.side / 2;
                    os << "<rect x=\"" << x << "\" y=\"" << m_scale_y.map(y) << "\" ";
                    os << "rx=\"" << element.corner_radious << "\" ";
                    os << "width=\"" << element.side << "\" height=\"" << element.side
                       << "\" fill=\"" << color_to_string(element.color) << "\" />\n";
                } else if constexpr (std::is_same_v<T, Circle2D>) {
                    os << "<circle cx=\"" << element.center.x << "\" cy=\""
                       << m_scale_y.map(element.center.y) << "\" ";
                    os << "r=\"" << element.radius << "\" fill=\"" << color_to_string(element.color)
                       << "\" />\n";
                } else if constexpr (std::is_same_v<T, Line2D>) {
                    os << "<line x1=\"" << element.p1.x << "\" y1=\"" << m_scale_y.map(element.p1.y)
                       << "\" ";
                    os << "x2=\"" << element.p2.x << "\" y2=\"" << m_scale_y.map(element.p2.y)
                       << "\" ";
                    os << "style=\"stroke:" << color_to_string(element.color)
                       << ";stroke-width:2\" />\n";
                } else if constexpr (std::is_same_v<T, Polygon2D>) {
                    os << "<polygon points=\"";
                    for (const Point2D& point : element.points)
                        os << point.x << "," << m_scale_y.map(point.y) << " ";
                    os << "\" style=\"fill:white;stroke:" << color_to_string(element.color)
                       << ";stroke-width:2\" />\n";
                } else if constexpr (std::is_same_v<T, Path2D>) {
                    os << "<path d=\"";
                    for (size_t i = 0; i < element.points.size(); i++) {
                        if (i == 0)
                            os << "M" << element.points[i].x << ","
                               << m_scale_y.map(element.points[i].y) << " ";
                        else
                            os << "L" << element.points[i].x << ","
                               << m_scale_y.map(element.points[i].y) << " ";
                    }
                    os << "\" style=\"fill:none;stroke:" << color_to_string(element.color)
                       << ";stroke-width:1\" />\n";
                } else if constexpr (std::is_same_v<T, SmoothPath2D>) {
                    os << "<path d=\"";
                    for (size_t i = 0; i < element.path.points.size(); i++) {
                        if (i == 0)
                            os << "M" << element.path.points[i].x << ","
                               << m_scale_y.map(element.path.points[i].y) << " ";
                        else
                            os << "T" << element.path.points[i].x << ","
                               << m_scale_y.map(element.path.points[i].y) << " ";
                    }
                    os << "\" style=\"fill:none;stroke:" << color_to_string(element.color)
                       << ";stroke-width:1\" />\n";
                }
            },
            element
        );
    }
}

const std::vector<DrawElement>& Drawer::get_elements() const { return m_elements; }

std::string Drawer::to_string() const {
    std::stringstream ss;
    render_to_stream(ss);
    ss << "</svg>\n";
    return ss.str();
}

std::expected<void, std::string> Drawer::save_to_file(std::filesystem::path path) const {
    std::ofstream svg_file(path);
    if (!svg_file.is_open()) {
        return std::unexpected(
            std::format("SvgDrawer::save_to_file: could not open file {}", path.string())
        );
    }
    render_to_stream(svg_file);
    svg_file << "</svg>\n";
    return {};
}

} // namespace domus::drawing