#include "domus/torus/mapping.hpp"

#include <cmath>

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/drawing/polygon.hpp"

namespace domus::torus::mapper {
using namespace domus::drawing;

// Convert 2D rectangle coordinates (x,y) to 3D torus coordinates (x,y,z)
Point3D TorusMapping::map_rectangle_to_torus(const Point2D& point) {
    // Map x,y (in range 0-1) to u,v parameters (in range 0-2π)
    const double u = 2.0f * M_PI * point.x;
    const double v = 2.0f * M_PI * point.y;

    // Parametric equations of the torus
    const double x = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * cos(u);
    const double y = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * sin(u);
    const double z = MINOR_RADIUS * sin(v);

    return {x, y, z};
}

void TorusMapping::update_point(size_t index, drawing::Point2D point) {
    m_rectangle_points[index] = point;
}

void TorusMapping::set_point_color(size_t index, ColorRGB color) {
    m_rectangle_points_color[index] = color;
}

void TorusMapping::set_point_hidden(size_t index) { m_is_rectangle_point_hidden[index] = true; }

void TorusMapping::set_point_shown(size_t index) { m_is_rectangle_point_hidden[index] = false; }

const std::vector<drawing::Point2D>& TorusMapping::get_points() const { return m_rectangle_points; }

size_t TorusMapping::add_point(drawing::Point2D point, size_t id) {
    m_rectangle_points.push_back(point);
    m_rectangle_points_color.push_back(SPHERE_DEFAULT_COLOR);
    m_is_rectangle_point_hidden.push_back(false);
    m_point_to_node_id.push_back(id);
    m_torus_points.push_back(map_rectangle_to_torus(point));
    return m_rectangle_points.size() - 1;
}

size_t TorusMapping::add_line(size_t start_index, size_t end_index) {
    m_rectangle_lines.emplace_back(start_index, end_index);
    m_rectangle_lines_color.push_back(EDGE_DEFAULT_COLOR);
    return m_rectangle_lines.size() - 1;
}

void TorusMapping::set_line_color(size_t index, ColorRGB color) {
    m_rectangle_lines_color[index] = color;
}

void TorusMapping::add_polygon(std::vector<size_t> polygon) {
    m_rectangle_polygons.push_back(polygon);
    m_rectangle_polygons_color.push_back(POLYGON_DEFAULT_COLOR);
}

void TorusMapping::set_polygon_color(size_t index, ColorRGB color) {
    m_rectangle_polygons_color[index] = color;
}

void TorusMapping::visualize() {
    return;
    for (Point2D point : m_rectangle_points) {
        DOMUS_ASSERT(
            point.x <= 1.0 && point.x >= 0.0,
            "TorusMapping::visualize: point x coordinate out of bounds"
        );
        DOMUS_ASSERT(
            point.y <= 1.0 && point.y >= 0.0,
            "TorusMapping::visualize: point y coordinate out of bounds"
        );
    }

    for (auto [i_0, i_1] : m_rectangle_lines) {
        DOMUS_ASSERT(
            i_0 < m_rectangle_points.size(),
            "TorusMapping::visualize: line start index out of bounds"
        );
        DOMUS_ASSERT(
            i_1 < m_rectangle_points.size(),
            "TorusMapping::visualize: line start index out of bounds"
        );
    }

    for (const auto& poly_indices : m_rectangle_polygons) {
        for (size_t idx : poly_indices) {
            DOMUS_ASSERT(
                idx < m_rectangle_points.size(),
                "TorusMapping::visualize: polygon point index out of bounds"
            );
        }
    }

    visualize_torus();
}

} // namespace domus::torus::mapper
