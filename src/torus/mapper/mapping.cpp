#include "domus/torus/mapping.hpp"

#include <cmath>

#include "domus/core/domus_debug.hpp"
#include "domus/drawing/draw_elements.hpp"

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

bool is_in_bounds(const Point2D& point) {
    if (point.x > 1.0 || point.x < 0.0)
        return false;
    if (point.y > 1.0 || point.y < 0.0)
        return false;
    return true;
}

void TorusMapping::add_circle(const Circle2D& circle, size_t node_id) {
    DOMUS_ASSERT(
        is_in_bounds(circle.center),
        "TorusMapping::add_circle: circle center out of bounds"
    );
    m_circles.push_back(circle);
    m_circle_to_node_id.push_back(node_id);
}

void TorusMapping::add_line(const drawing::Line2D& line) {
    DOMUS_ASSERT(is_in_bounds(line.p1), "TorusMapping::add_line: line p1 out of bounds");
    DOMUS_ASSERT(is_in_bounds(line.p2), "TorusMapping::add_line: line p2 out of bounds");
    m_lines.push_back(line);
}

void TorusMapping::add_polygon(const drawing::Polygon2D& polygon) {
    DOMUS_ASSERT(
        polygon.fill_color.has_value(),
        "TorusMapping::add_polygon: polygon does not have fill color"
    );
    for (const auto& point : polygon.points)
        DOMUS_ASSERT(is_in_bounds(point), "TorusMapping::add_polygon: point out of bounds");
    m_polygons.push_back(polygon);
}

} // namespace domus::torus::mapper
