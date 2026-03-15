#include "domus/drawing/polygon.hpp"

#include <algorithm>
#include <cmath>
#include <stddef.h>

#include "../core/domus_assert.hpp"

Point2D Point2D::operator+(const Point2D& other) const {
    return {x_m + other.x_m, y_m + other.y_m};
}

Point2D Point2D::operator-(const Point2D& other) const {
    return {x_m + other.x_m, y_m + other.y_m};
}

Point2D Point2D::operator*(const double scalar) const { return {x_m * scalar, y_m * scalar}; }

Point2D Point2D::operator/(const double scalar) const { return {x_m / scalar, y_m / scalar}; }

bool Point2D::operator==(const Point2D& other) const {
    return x_m == other.x_m && y_m == other.y_m;
}

bool Point2D::operator!=(const Point2D& other) const { return !(*this == other); }

bool Point2D::operator<(const Point2D& p) const {
    return x_m < p.x_m || (x_m == p.x_m && y_m < p.y_m);
}

void Path2D::addPoint(const Point2D& p) { points.push_back(p); }

bool Line2D::operator==(const Line2D& other) const {
    return p1_m == other.p1_m && p2_m == other.p2_m;
}

bool Line2D::operator!=(const Line2D& other) const { return !(*this == other); }

Circle2D::Circle2D(const Point2D& center, const double radius)
    : center_m(center), radius_m(radius) {}

Point2D Circle2D::getCenter() const { return center_m; }

double Circle2D::getRadius() const { return radius_m; }

void Circle2D::setLabel(const std::string_view label) { label_m = label; }

bool Circle2D::hasLabel() const { return !label_m.empty(); }

const std::string_view Circle2D::getLabel() const {
    DOMUS_ASSERT(hasLabel(), "Circle2D::getLabel: circle does not have a label");
    return label_m;
}

void Circle2D::setColor(const std::string_view color) { color_m = color; }

bool Circle2D::hasColor() const { return !color_m.empty(); }

const std::string_view Circle2D::getColor() const {
    DOMUS_ASSERT(hasColor(), "Circle2D::getColor: circle does not have a color");
    return color_m;
}

Square2D::Square2D(const Point2D& center, const double side) : center_m(center), side_m(side) {}

Point2D Square2D::getCenter() const { return center_m; }

double Square2D::getSide() const { return side_m; }

void Square2D::setLabel(const std::string& label) { this->label_m = label; }

bool Square2D::hasLabel() const { return label_m.has_value(); }

std::optional<std::string> Square2D::getLabel() const { return label_m; }

void Square2D::setColor(const std::string& color) { this->color_m = color; }

bool Square2D::hasColor() const { return color_m.has_value(); }

std::optional<std::string> Square2D::getColor() const { return color_m; }

Point2D::Point2D(const double x, const double y) : x_m(x), y_m(y) {}

double Point2D::distance(const Point2D& other) const {
    return sqrt((x_m - other.x_m) * (x_m - other.x_m) + (y_m - other.y_m) * (y_m - other.y_m));
}

Line2D::Line2D(const Point2D& p1, const Point2D& p2) : p1_m(p1), p2_m(p2) {}

bool Line2D::isPointOnLine(const Point2D& p) const {
    double crossProduct =
        (p.y_m - p1_m.y_m) * (p2_m.x_m - p1_m.x_m) - (p.x_m - p1_m.x_m) * (p2_m.y_m - p1_m.y_m);
    if (std::abs(crossProduct) > 1e-7)
        return false; // Not on the line
    double dotProduct =
        (p.x_m - p1_m.x_m) * (p2_m.x_m - p1_m.x_m) + (p.y_m - p1_m.y_m) * (p2_m.y_m - p1_m.y_m);
    if (dotProduct < 0)
        return false; // Not on the segment
    double squaredLength = (p2_m.x_m - p1_m.x_m) * (p2_m.x_m - p1_m.x_m) +
                           (p2_m.y_m - p1_m.y_m) * (p2_m.y_m - p1_m.y_m);
    if (dotProduct > squaredLength)
        return false; // Not on the segment
    return true;
}

bool Line2D::isIntersecting(const Line2D& l) const {
    double denominator = (p2_m.y_m - p1_m.y_m) * (l.p2_m.x_m - l.p1_m.x_m) -
                         (p2_m.x_m - p1_m.x_m) * (l.p2_m.y_m - l.p1_m.y_m);
    if (denominator == 0)
        return false; // Lines are parallel
    double ua = ((p2_m.x_m - p1_m.x_m) * (l.p1_m.y_m - p1_m.y_m) -
                 (p2_m.y_m - p1_m.y_m) * (l.p1_m.x_m - p1_m.x_m)) /
                denominator;
    double ub = ((l.p2_m.x_m - l.p1_m.x_m) * (l.p1_m.y_m - p1_m.y_m) -
                 (l.p2_m.y_m - l.p1_m.y_m) * (l.p1_m.x_m - p1_m.x_m)) /
                denominator;
    return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}

Polygon2D::Polygon2D(const std::vector<Point2D>& points) {
    DOMUS_ASSERT(points.size() >= 3, "Polygon2D::Polygon2D: Polygon must have at least 3 points");
    for (const auto& p : points) {
        m_points.emplace_back(p.x_m, p.y_m);
    }
}

std::vector<Point2D>& Polygon2D::getPoints() { return m_points; }

bool Polygon2D::isOnBoundary(const Point2D& p) const {
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        if (p1.x_m == p2.x_m) {
            if (p.x_m == p1.x_m && p.y_m >= std::min(p1.y_m, p2.y_m) &&
                p.y_m <= std::max(p1.y_m, p2.y_m))
                return true;
        } else {
            double m = (p2.y_m - p1.y_m) / (p2.x_m - p1.x_m);
            double c = p1.y_m - m * p1.x_m;
            double y = m * p.x_m + c;
            if (p.y_m == y && p.x_m >= std::min(p1.x_m, p2.x_m) &&
                p.x_m <= std::max(p1.x_m, p2.x_m))
                return true;
        }
    }
    return false;
}

bool Polygon2D::isInside(const Point2D& p) const {
    size_t count = 0;
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        if (p == p1)
            return true;
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        if (p1.y_m == p2.y_m)
            continue;
        if (p.y_m < std::min(p1.y_m, p2.y_m))
            continue;
        if (p.y_m >= std::max(p1.y_m, p2.y_m))
            continue;
        double x = (p.y_m - p1.y_m) * (p2.x_m - p1.x_m) / (p2.y_m - p1.y_m) + p1.x_m;
        if (x > p.x_m)
            count++;
    }
    return count % 2 == 1;
}

bool Polygon2D::isInside(const Line2D& l) const {
    if (!isInside(l.p1_m) || !isInside(l.p2_m))
        return false;
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        const Line2D edge(p1, p2);
        if (l.isIntersecting(edge)) {
            if (!(l.isPointOnLine(p1) || l.isPointOnLine(p2)))
                return false;
        }
    }
    if (isOnBoundary(l.p1_m) && isOnBoundary(l.p2_m)) {
        const Point2D m = (l.p1_m + l.p2_m) / 2.0;
        if (!isInside(m))
            return false;
    }
    return true;
}

// Calcola il determinante tra tre punti (p, q, r)
// Se > 0, indica una svolta a sinistra (convesso);
// se < 0, indica una svolta a destra (concavo);
// se == 0, sono collineari
double cross(const Point2D& p, const Point2D& q, const Point2D& r) {
    return (q.x_m - p.x_m) * (r.y_m - p.y_m) - (q.y_m - p.y_m) * (r.x_m - p.x_m);
}

// Funzione per calcolare il contorno convesso
std::vector<Point2D> Polygon2D::computeConvexHull() const {
    std::vector<Point2D> points(m_points);
    // Ordina i punti in base all'ordinamento lessicografico
    std::sort(points.begin(), points.end());
    std::vector<Point2D> hull;
    // Costruzione della metà inferiore del contorno
    for (const auto& p : points) {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
            hull.pop_back();
        hull.push_back(p);
    }
    // Costruzione della metà superiore del contorno
    const size_t lowerSize = hull.size();
    for (size_t i = points.size(); i > 0; --i) {
        while (hull.size() > lowerSize &&
               cross(hull[hull.size() - 2], hull[hull.size() - 1], points[i - 1]) <= 0)
            hull.pop_back();
        hull.push_back(points[i - 1]);
    }
    // Rimuove l'ultimo punto poiché è uguale al primo
    hull.pop_back();
    return hull;
}