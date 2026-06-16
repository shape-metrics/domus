#pragma once

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
    void add_point(const Point2D& p);
};

struct Line2D {
    Point2D m_p1;
    Point2D m_p2;

    Line2D(const Point2D& p1, const Point2D& p2);
    bool is_point_on_line(const Point2D& p) const;
    bool is_intersecting(const Line2D& l) const;
    bool operator==(const Line2D& other) const;
    bool operator!=(const Line2D& other) const;
};

struct Polygon2D {
    std::vector<Point2D> points;

    explicit Polygon2D(const std::vector<Point2D>& points);
    bool is_on_boundary(const Point2D& p) const;
    bool is_inside(const Point2D& p) const;
    bool is_inside(const Line2D& l) const;
    std::vector<Point2D> compute_convex_hull() const;
};

struct Circle2D {
    Point2D center;
    double radius;
};

struct Square2D {
    Point2D center;
    double side;
};

struct RoundSquare2D {
    Point2D center;
    double side;
    double corner_radious;
};

} // namespace domus::drawing