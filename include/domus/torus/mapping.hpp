#pragma once

#include <expected>
#include <filesystem>
#include <utility>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/drawing/polygon.hpp"

namespace domus::torus::mapper {
using color::ColorRGB;

// Torus parameters
const double MAJOR_RADIUS = 3.0;      // R - distance from center of tube to center of torus
const double MINOR_RADIUS = 1.0;      // r - radius of the tube itself
const int TORUS_SEGMENTS_MAJOR = 128; // Resolution of the torus
const int TORUS_SEGMENTS_MINOR = 64;  // Resolution of the torus
const ColorRGB TORUS_COLOR = GRAY_RGB;

// Sphere (drawn vertices on the torus) parameters
const double SPHERE_RADIUS = 0.1;
const int SPHERE_SLICES = 8; // Detail level of spheres
const ColorRGB SPHERE_DEFAULT_COLOR = RED_RGB;

// Cylinder (drawn edges on the torus) parameters
const ColorRGB EDGE_DEFAULT_COLOR = NAVY_RGB;
const double EDGE_RADIUS = 0.02;
const int EDGE_SLICES = 50; // Detail level of cylinders

// Rectangle variables
const float RECTANGLE_POINT_SIZE = 15.0f; // Size of vertices in the 2D view
const float RECTANGLE_EDGE_WIDTH = 5.0f;  // Width of edges in the 2D view

// Polygons variables
const int POLYGON_GRID_RES_U = 512;
const int POLYGON_GRID_RES_V = 256;
const ColorRGB POLYGON_DEFAULT_COLOR = TEAL_RGB;

// Labels parameters
const ColorRGB LABELS_TEXT_COLOR = WHITE_RGB;
const ColorRGB LABELS_BACKGROUND_COLOR = {0.2f, 0.2f, 0.2f};

class TorusMapping {
    struct PolygonMesh {
        std::vector<drawing::Point3D> quads_3d;
        std::vector<drawing::Point2D> quads_2d;
    };

    std::vector<drawing::Point2D> m_rectangle_points;
    std::vector<ColorRGB> m_rectangle_points_color;
    std::vector<bool> m_is_rectangle_point_hidden;
    std::vector<size_t> m_point_to_node_id;
    std::vector<drawing::Point3D> m_torus_points;

    std::vector<std::pair<size_t, size_t>> m_rectangle_lines;
    std::vector<ColorRGB> m_rectangle_lines_color;

    std::vector<std::vector<size_t>> m_rectangle_polygons;
    std::vector<ColorRGB> m_rectangle_polygons_color;
    std::vector<PolygonMesh> m_cached_polygon_meshes;
    std::vector<ColorRGB> m_cached_polygon_meshes_color;

    void draw_rectangle() const;
    void display() const;
    void precompute_polygons();
    void visualize_torus();

  public:
    size_t add_point(drawing::Point2D point, size_t id);
    void update_point(size_t index, drawing::Point2D point);
    void set_point_color(size_t index, ColorRGB color);
    void set_point_hidden(size_t index);
    void set_point_shown(size_t index);
    const std::vector<drawing::Point2D>& get_points() const;

    size_t add_line(size_t start_index, size_t end_index);
    void set_line_color(size_t index, ColorRGB color);

    void add_polygon(std::vector<size_t> polygon);
    void set_polygon_color(size_t index, ColorRGB color);

    static drawing::Point3D map_rectangle_to_torus(const drawing::Point2D& point);
    void visualize();

    std::expected<void, std::string> save_to_file(std::filesystem::path path);
    static std::expected<TorusMapping, std::string> load_from_file(std::filesystem::path path);

    friend void display_callback();
};

} // namespace domus::torus::mapper