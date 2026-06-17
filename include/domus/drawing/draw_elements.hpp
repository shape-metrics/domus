#pragma once

#include "domus/core/color.hpp"
#include <string>
#include <vector>

namespace domus::drawing {

struct Point2D {
    double x = 0.0;
    double y = 0.0;

    double distance(const Point2D& other) const;
    Point2D operator+(const Point2D& other) const;
    Point2D operator-(const Point2D& other) const;
    Point2D operator*(const double scalar) const;
    Point2D operator/(const double scalar) const;
    bool operator==(const Point2D& other) const;
    bool operator!=(const Point2D& other) const;
    bool operator<(const Point2D& p) const;
};

struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    double distance(const Point3D& other) const;
    Point3D operator+(const Point3D& other) const;
    Point3D operator-(const Point3D& other) const;
    Point3D operator*(const double scalar) const;
    Point3D operator/(const double scalar) const;
    bool operator==(const Point3D& other) const;
    bool operator!=(const Point3D& other) const;
};

struct Path2D {
    std::vector<Point2D> points;
    color::ColorRGB color;

    void add_point(const Point2D& p);
};

struct Line2D {
    Point2D p1;
    Point2D p2;
    color::ColorRGB color;

    bool is_point_on_line(const Point2D& p) const;
    bool is_intersecting(const Line2D& l) const;
    bool operator==(const Line2D& other) const;
    bool operator!=(const Line2D& other) const;
};

struct Polygon2D {
    std::vector<Point2D> points;
    color::ColorRGB color;
    std::optional<color::ColorRGB> fill_color;

    explicit Polygon2D(
        const std::vector<Point2D>& points,
        color::ColorRGB color,
        std::optional<color::ColorRGB> fill_color
    );
    bool is_on_boundary(const Point2D& p) const;
    bool is_inside(const Point2D& p) const;
    bool is_inside(const Line2D& l) const;
    std::vector<Point2D> compute_convex_hull() const;
};

struct Circle2D {
    Point2D center;
    double radius;
    color::ColorRGB color;
};

struct Square2D {
    Point2D center;
    double side;
    color::ColorRGB color;
};

struct RoundSquare2D {
    Point2D center;
    double side;
    double corner_radious;
    color::ColorRGB color;
};

struct Text2D {
    std::string text;
    Point2D center;
};

struct SmoothPath2D {
    Path2D path;
    color::ColorRGB color;
};

} // namespace domus::drawing