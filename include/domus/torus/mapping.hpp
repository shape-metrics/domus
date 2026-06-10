#pragma once

#include <utility>
#include <vector>

#include "domus/drawing/polygon.hpp"

namespace domus::torus::mapper {

class TorusMapping {
    std::vector<drawing::Point2D> m_rectangle_points;
    std::vector<std::pair<size_t, size_t>> m_rectangle_lines;
    std::vector<drawing::Polygon2D> m_rectangle_polygons;

  public:
    void add_point(drawing::Point2D point);
    void add_line(size_t start_index, size_t end_index);
    void add_polygon(drawing::Polygon2D polygon);
    void visualize() const;
};

} // namespace domus::torus::mapper