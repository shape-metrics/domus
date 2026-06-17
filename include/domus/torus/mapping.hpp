#pragma once

#include <expected>
#include <filesystem>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/drawing/draw_elements.hpp"

namespace domus::torus::mapper {

// Torus parameters
const double MAJOR_RADIUS = 3.0;      // R - distance from center of tube to center of torus
const double MINOR_RADIUS = 1.0;      // r - radius of the tube itself
const int TORUS_SEGMENTS_MAJOR = 128; // Resolution of the torus
const int TORUS_SEGMENTS_MINOR = 64;  // Resolution of the torus
const color::ColorRGB TORUS_COLOR = GRAY_RGB;

// Sphere (drawn vertices on the torus) parameters
const int SPHERE_SLICES = 8; // Detail level of spheres

// Cylinder (drawn edges on the torus) parameters
const double EDGE_RADIUS = 0.02;
const int EDGE_SLICES = 50; // Detail level of cylinders

// Rectangle variables
const float RECTANGLE_EDGE_WIDTH = 5.0f; // Width of edges in the 2D view

// Polygons variables
const int POLYGON_GRID_RES_U = 512;
const int POLYGON_GRID_RES_V = 256;

// Labels parameters
const color::ColorRGB LABELS_TEXT_COLOR = WHITE_RGB;
const color::ColorRGB LABELS_BACKGROUND_COLOR = {0.2f, 0.2f, 0.2f};

class TorusMapping {
    struct PolygonMesh {
        std::vector<drawing::Point3D> quads_3d;
        std::vector<drawing::Point2D> quads_2d;
        color::ColorRGB color;
        PolygonMesh(color::ColorRGB color) : color(color) {}
    };

    std::vector<drawing::Circle2D> m_circles;
    std::vector<size_t> m_circle_to_node_id;

    std::vector<drawing::Line2D> m_lines;

    std::vector<drawing::Polygon2D> m_polygons;
    std::vector<PolygonMesh> m_cached_polygon_meshes;

    void draw_rectangle() const;
    void display() const;
    void precompute_polygons();
    void draw_torus_line(
        const drawing::Point2D& start, const drawing::Point2D& end, const color::ColorRGB& color
    ) const;

    static drawing::Point3D map_rectangle_to_torus(const drawing::Point2D& point);

  public:
    void add_circle(const drawing::Circle2D& circle, size_t node_id);
    void add_line(const drawing::Line2D& line);
    void add_polygon(const drawing::Polygon2D& polygon);

    std::expected<void, std::string> save_to_file(std::filesystem::path path);
    static std::expected<TorusMapping, std::string> load_from_file(std::filesystem::path path);

    friend void display_callback();

    void visualize();
};

} // namespace domus::torus::mapper