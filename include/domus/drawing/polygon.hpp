#pragma once

#include <vector>

namespace domus::drawing {

class Point2D {
  public:
    double x;
    double y;

    Point2D(double x, double y);
    double distance(const Point2D& other) const;
    Point2D operator+(const Point2D& other) const;
    Point2D operator-(const Point2D& other) const;
    Point2D operator*(const double scalar) const;
    Point2D operator/(const double scalar) const;
    bool operator==(const Point2D& other) const;
    bool operator!=(const Point2D& other) const;
    bool operator<(const Point2D& p) const;
};

class Point3D {
  public:
    double x;
    double y;
    double z;

    Point3D(double x, double y, double z);
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

class Line2D {
  public:
    Point2D m_p1;
    Point2D m_p2;

    Line2D(const Point2D& p1, const Point2D& p2);
    bool is_point_on_line(const Point2D& p) const;
    bool is_intersecting(const Line2D& l) const;
    bool operator==(const Line2D& other) const;
    bool operator!=(const Line2D& other) const;
};

class Polygon2D {
    std::vector<Point2D> m_points;

  public:
    explicit Polygon2D(const std::vector<Point2D>& points);
    const std::vector<Point2D>& get_points() const;
    bool is_on_boundary(const Point2D& p) const;
    bool is_inside(const Point2D& p) const;
    bool is_inside(const Line2D& l) const;
    std::vector<Point2D> compute_convex_hull() const;
};

class Circle2D {
    Point2D m_center;
    double m_radius;

  public:
    Circle2D(const Point2D& center, const double radius);
    const Point2D& get_center() const;
    double get_radius() const;
};

class Square2D {
    Point2D m_center;
    double m_side;

  public:
    Square2D(const Point2D& center, const double side);
    const Point2D& get_center() const;
    double get_side() const;
};

class RoundSquare2D {
    Square2D m_square;
    double m_corner_radious;

  public:
    RoundSquare2D(const Point2D& center, const double side, const double corner_radious);
    const Point2D& get_center() const;
    double get_side() const;
    double get_corner_radious() const;
};

} // namespace domus::drawing