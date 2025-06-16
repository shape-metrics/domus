#ifndef MY_POLYGON_H
#define MY_POLYGON_H

#include <iostream>
#include <optional>
#include <string>
#include <vector>

class Point2D {
 public:
  double x, y;
  Point2D(double x, double y);
  double distance(const Point2D& other);
  Point2D operator+(const Point2D& other) const {
    return Point2D(x + other.x, y + other.y);
  }
  Point2D operator-(const Point2D& other) const {
    return Point2D(x + other.x, y + other.y);
  }
  Point2D operator*(const double scalar) const {
    return Point2D(x * scalar, y * scalar);
  }
  Point2D operator/(const double scalar) const {
    return Point2D(x / scalar, y / scalar);
  }
  bool operator==(const Point2D& other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Point2D& other) const { return !(*this == other); }
  // Operatore di confronto lessicografico (prima x, poi y)
  bool operator<(const Point2D& p) const {
    return x < p.x || (x == p.x && y < p.y);
  }
  friend std::ostream& operator<<(std::ostream& os, const Point2D& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
  }
};

class Path2D {
 public:
  std::vector<Point2D> points;
  void addPoint(Point2D& p) { points.push_back(p); }
};

class Line2D {
 public:
  Point2D p1, p2;
  Line2D(Point2D& p1, Point2D& p2);
  bool isPointOnLine(Point2D& p);
  bool isIntersecting(Line2D& l);
  bool operator==(const Line2D& other) const {
    return p1 == other.p1 && p2 == other.p2;
  }
  bool operator!=(const Line2D& other) const { return !(*this == other); }
  friend std::ostream& operator<<(std::ostream& os, const Line2D& l) {
    os << "[" << l.p1 << " - " << l.p2 << "]";
    return os;
  }
};

class Polygon2D {
 private:
  std::vector<Point2D> m_points;

 public:
  Polygon2D(std::vector<Point2D>& points);
  std::vector<Point2D>& getPoints();
  bool isOnBoundary(Point2D& p);
  bool isInside(Point2D& p);
  bool isInside(Line2D& l);
  std::vector<Point2D> computeConvexHull();
};

class Circle2D {
 private:
  Point2D center;
  double radius;
  std::optional<std::string> label;
  std::optional<std::string> color;

 public:
  Circle2D(Point2D& center, double radius) : center(center), radius(radius) {}
  Point2D getCenter() const { return center; }
  double getRadius() const { return radius; }
  void setLabel(const std::string& label) { this->label = label; }
  bool hasLabel() const { return label.has_value(); }
  std::string getLabel() const {
    if (!label.has_value()) throw std::runtime_error("Circle has no label");
    return *label;
  }
  void setColor(const std::string& color) { this->color = color; }
  bool hasColor() const { return color.has_value(); }
  std::string getColor() const {
    if (!color.has_value()) throw std::runtime_error("Circle has no color");
    return *color;
  }
};

class Square2D {
 private:
  Point2D center;
  double side;
  std::optional<std::string> label;
  std::optional<std::string> color;

 public:
  Square2D(Point2D& center, double side) : center(center), side(side) {}
  Point2D getCenter() const { return center; }
  double getSide() const { return side; }
  void setLabel(const std::string& label) { this->label = label; }
  bool hasLabel() const { return label.has_value(); }
  std::string getLabel() const {
    if (!label.has_value()) throw std::runtime_error("Square has no label");
    return *label;
  }
  void setColor(const std::string& color) { this->color = color; }
  bool hasColor() const { return color.has_value(); }
  std::string getColor() const {
    if (!color.has_value()) throw std::runtime_error("Square has no color");
    return *color;
  }
};

#endif