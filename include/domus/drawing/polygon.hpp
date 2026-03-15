#pragma once

#include <optional>
#include <string>
#include <vector>

class Point2D {
  public:
    double x_m, y_m;
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

struct Path2D {
    std::vector<Point2D> points;
    void addPoint(const Point2D& p);
};

class Line2D {
  public:
    Point2D p1_m, p2_m;
    Line2D(const Point2D& p1, const Point2D& p2);
    bool isPointOnLine(const Point2D& p) const;
    bool isIntersecting(const Line2D& l) const;
    bool operator==(const Line2D& other) const;
    bool operator!=(const Line2D& other) const;
};

class Polygon2D {
    std::vector<Point2D> m_points;

  public:
    explicit Polygon2D(const std::vector<Point2D>& points);
    std::vector<Point2D>& getPoints();
    bool isOnBoundary(const Point2D& p) const;
    bool isInside(const Point2D& p) const;
    bool isInside(const Line2D& l) const;
    std::vector<Point2D> computeConvexHull() const;
};

class Circle2D {
    Point2D center_m;
    double radius_m;
    std::string label_m{};
    std::string color_m{};

  public:
    Circle2D(const Point2D& center, const double radius);
    Point2D getCenter() const;
    double getRadius() const;
    void setLabel(const std::string_view label);
    bool hasLabel() const;
    const std::string_view getLabel() const;
    void setColor(const std::string_view color);
    bool hasColor() const;
    const std::string_view getColor() const;
};

class Square2D {
    Point2D center_m;
    double side_m;
    std::optional<std::string> label_m;
    std::optional<std::string> color_m;

  public:
    Square2D(const Point2D& center, const double side);
    Point2D getCenter() const;
    double getSide() const;
    void setLabel(const std::string& label);
    bool hasLabel() const;
    std::optional<std::string> getLabel() const;
    void setColor(const std::string& color);
    bool hasColor() const;
    std::optional<std::string> getColor() const;
};