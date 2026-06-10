#pragma once

#include <utility>
#include <vector>

#include "domus/drawing/polygon.hpp"
#include "domus/drawing/rgb_color.hpp"

namespace domus::torus::mapper {

class TorusMapping {
    struct PolygonMesh {
        std::vector<drawing::Point3D> quads_3d;
        std::vector<drawing::Point2D> quads_2d;
    };

    std::vector<drawing::Point2D> m_rectangle_points;
    std::vector<ColorRGB> m_rectangle_points_color;

    std::vector<std::pair<size_t, size_t>> m_rectangle_lines;
    std::vector<ColorRGB> m_rectangle_lines_color;

    std::vector<drawing::Polygon2D> m_rectangle_polygons;
    std::vector<ColorRGB> m_rectangle_polygons_color;
    std::vector<PolygonMesh> m_cached_polygon_meshes;

    void draw_rectangle() const;
    void display() const;
    void precompute_polygons();
    void visualize_torus();

  public:
    void add_point(drawing::Point2D point);
    void add_line(size_t start_index, size_t end_index);
    void set_line_color(size_t index, ColorRGB color);
    void add_polygon(drawing::Polygon2D polygon);
    void visualize();
    friend void display_callback();
};

} // namespace domus::torus::mapper