#ifndef MY_POLYGON_H
#define MY_POLYGON_H

#include <iostream>
#include <optional>
#include <string>
#include <vector>

class Point2D {
  public:
    double x_m, y_m;
    Point2D(double x, double y);
    double distance(const Point2D& other) const;
    Point2D operator+(const Point2D& other) const { return {x_m + other.x_m, y_m + other.y_m}; }
    Point2D operator-(const Point2D& other) const { return {x_m + other.x_m, y_m + other.y_m}; }
    Point2D operator*(const double scalar) const { return {x_m * scalar, y_m * scalar}; }
    Point2D operator/(const double scalar) const { return {x_m / scalar, y_m / scalar}; }
    bool operator==(const Point2D& other) const { return x_m == other.x_m && y_m == other.y_m; }
    bool operator!=(const Point2D& other) const { return !(*this == other); }
    // Operatore di confronto lessicografico (prima x, poi y)
    bool operator<(const Point2D& p) const { return x_m < p.x_m || (x_m == p.x_m && y_m < p.y_m); }
    friend std::ostream& operator<<(std::ostream& os, const Point2D& p) {
        os << "(" << p.x_m << ", " << p.y_m << ")";
        return os;
    }
};

class Path2D {
  public:
    std::vector<Point2D> points;
    void addPoint(const Point2D& p) { points.push_back(p); }
};

class Line2D {
  public:
    Point2D p1_m, p2_m;
    Line2D(const Point2D& p1, const Point2D& p2);
    bool isPointOnLine(const Point2D& p) const;
    bool isIntersecting(const Line2D& l) const;
    bool operator==(const Line2D& other) const { return p1_m == other.p1_m && p2_m == other.p2_m; }
    bool operator!=(const Line2D& other) const { return !(*this == other); }
    friend std::ostream& operator<<(std::ostream& os, const Line2D& l) {
        os << "[" << l.p1_m << " - " << l.p2_m << "]";
        return os;
    }
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
    std::optional<std::string> label_m;
    std::optional<std::string> color_m;

  public:
    Circle2D(const Point2D& center, const double radius) : center_m(center), radius_m(radius) {}
    Point2D getCenter() const { return center_m; }
    double getRadius() const { return radius_m; }
    void setLabel(const std::string& label) { this->label_m = label; }
    bool hasLabel() const { return label_m.has_value(); }
    std::optional<std::string> getLabel() const { return label_m; }
    void setColor(const std::string& color) { this->color_m = color; }
    bool hasColor() const { return color_m.has_value(); }
    std::optional<std::string> getColor() const { return color_m; }
};

class Square2D {
    Point2D center_m;
    double side_m;
    std::optional<std::string> label_m;
    std::optional<std::string> color_m;

  public:
    Square2D(const Point2D& center, const double side) : center_m(center), side_m(side) {}
    Point2D getCenter() const { return center_m; }
    double getSide() const { return side_m; }
    void setLabel(const std::string& label) { this->label_m = label; }
    bool hasLabel() const { return label_m.has_value(); }
    std::optional<std::string> getLabel() const { return label_m; }
    void setColor(const std::string& color) { this->color_m = color; }
    bool hasColor() const { return color_m.has_value(); }
    std::optional<std::string> getColor() const { return color_m; }
};

#endif