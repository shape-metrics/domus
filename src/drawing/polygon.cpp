#include "domus/drawing/polygon.hpp"

#include <algorithm>
#include <cmath>
#include <stddef.h>

#include "domus/core/domus_debug.hpp"

namespace domus::drawing {

Point2D::Point2D(const double x, const double y) : x(x), y(y) {}

double Point2D::distance(const Point2D& other) const {
    return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
}

Point2D Point2D::operator+(const Point2D& other) const { return {x + other.x, y + other.y}; }

Point2D Point2D::operator-(const Point2D& other) const { return {x - other.x, y - other.y}; }

Point2D Point2D::operator*(const double scalar) const { return {x * scalar, y * scalar}; }

Point2D Point2D::operator/(const double scalar) const { return {x / scalar, y / scalar}; }

bool Point2D::operator==(const Point2D& other) const { return x == other.x && y == other.y; }

bool Point2D::operator!=(const Point2D& other) const { return !(*this == other); }

bool Point2D::operator<(const Point2D& p) const { return x < p.x || (x == p.x && y < p.y); }

Point3D::Point3D(const double x, const double y, const double z) : x(x), y(y), z(z) {}

double Point3D::distance(const Point3D& other) const {
    return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
}

Point3D Point3D::operator+(const Point3D& other) const {
    return {x + other.x, y + other.y, z + other.z};
}

Point3D Point3D::operator-(const Point3D& other) const {
    return {x - other.x, y - other.y, z - other.z};
}

Point3D Point3D::operator*(const double scalar) const {
    return {x * scalar, y * scalar, z * scalar};
}

Point3D Point3D::operator/(const double scalar) const {
    return {x / scalar, y / scalar, z / scalar};
}

bool Point3D::operator==(const Point3D& other) const {
    return x == other.x && y == other.y && z == other.z;
}

void Path2D::add_point(const Point2D& p) { points.push_back(p); }

bool Line2D::operator==(const Line2D& other) const {
    return m_p1 == other.m_p1 && m_p2 == other.m_p2;
}

bool Line2D::operator!=(const Line2D& other) const { return !(*this == other); }

Circle2D::Circle2D(const Point2D& center, const double radius)
    : m_center(center), m_radius(radius) {}

const Point2D& Circle2D::get_center() const { return m_center; }

double Circle2D::get_radius() const { return m_radius; }

Square2D::Square2D(const Point2D& center, const double side) : m_center(center), m_side(side) {}

const Point2D& Square2D::get_center() const { return m_center; }

double Square2D::get_side() const { return m_side; }

RoundSquare2D::RoundSquare2D(const Point2D& center, const double side, const double corner_radious)
    : m_square(center, side), m_corner_radious(corner_radious) {}

const Point2D& RoundSquare2D::get_center() const { return m_square.get_center(); }

double RoundSquare2D::get_side() const { return m_square.get_side(); }

double RoundSquare2D::get_corner_radious() const { return m_corner_radious; }

Line2D::Line2D(const Point2D& p1, const Point2D& p2) : m_p1(p1), m_p2(p2) {}

bool Line2D::is_point_on_line(const Point2D& p) const {
    double crossProduct = (p.y - m_p1.y) * (m_p2.x - m_p1.x) - (p.x - m_p1.x) * (m_p2.y - m_p1.y);
    if (std::abs(crossProduct) > 1e-7)
        return false; // Not on the line
    double dotProduct = (p.x - m_p1.x) * (m_p2.x - m_p1.x) + (p.y - m_p1.y) * (m_p2.y - m_p1.y);
    if (dotProduct < 0)
        return false; // Not on the segment
    double squaredLength =
        (m_p2.x - m_p1.x) * (m_p2.x - m_p1.x) + (m_p2.y - m_p1.y) * (m_p2.y - m_p1.y);
    if (dotProduct > squaredLength)
        return false; // Not on the segment
    return true;
}

bool Line2D::is_intersecting(const Line2D& l) const {
    double denominator =
        (m_p2.y - m_p1.y) * (l.m_p2.x - l.m_p1.x) - (m_p2.x - m_p1.x) * (l.m_p2.y - l.m_p1.y);
    if (denominator == 0)
        return false; // Lines are parallel
    double ua =
        ((m_p2.x - m_p1.x) * (l.m_p1.y - m_p1.y) - (m_p2.y - m_p1.y) * (l.m_p1.x - m_p1.x)) /
        denominator;
    double ub = ((l.m_p2.x - l.m_p1.x) * (l.m_p1.y - m_p1.y) -
                 (l.m_p2.y - l.m_p1.y) * (l.m_p1.x - m_p1.x)) /
                denominator;
    return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}

Polygon2D::Polygon2D(const std::vector<Point2D>& points) {
    DOMUS_ASSERT(points.size() >= 3, "Polygon2D::Polygon2D: Polygon must have at least 3 points");
    for (const auto& p : points) {
        m_points.emplace_back(p.x, p.y);
    }
}

const std::vector<Point2D>& Polygon2D::get_points() const { return m_points; }

bool Polygon2D::is_on_boundary(const Point2D& p) const {
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        if (p1.x == p2.x) {
            if (p.x == p1.x && p.y >= std::min(p1.y, p2.y) && p.y <= std::max(p1.y, p2.y))
                return true;
        } else {
            double m = (p2.y - p1.y) / (p2.x - p1.x);
            double c = p1.y - m * p1.x;
            double y = m * p.x + c;
            if (p.y == y && p.x >= std::min(p1.x, p2.x) && p.x <= std::max(p1.x, p2.x))
                return true;
        }
    }
    return false;
}

bool Polygon2D::is_inside(const Point2D& p) const {
    size_t count = 0;
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        if (p == p1)
            return true;
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        if (p1.y == p2.y)
            continue;
        if (p.y < std::min(p1.y, p2.y))
            continue;
        if (p.y >= std::max(p1.y, p2.y))
            continue;
        double x = (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
        if (x > p.x)
            count++;
    }
    return count % 2 == 1;
}

bool Polygon2D::is_inside(const Line2D& l) const {
    if (!is_inside(l.m_p1) || !is_inside(l.m_p2))
        return false;
    for (size_t i = 0; i < m_points.size(); i++) {
        const Point2D& p1 = m_points[i];
        const Point2D& p2 = m_points[(i + 1) % m_points.size()];
        const Line2D edge(p1, p2);
        if (l.is_intersecting(edge)) {
            if (!(l.is_point_on_line(p1) || l.is_point_on_line(p2)))
                return false;
        }
    }
    if (is_on_boundary(l.m_p1) && is_on_boundary(l.m_p2)) {
        const Point2D m = (l.m_p1 + l.m_p2) / 2.0;
        if (!is_inside(m))
            return false;
    }
    return true;
}

// Calcola il determinante tra tre punti (p, q, r)
// Se > 0, indica una svolta a sinistra (convesso);
// se < 0, indica una svolta a destra (concavo);
// se == 0, sono collineari
double cross(const Point2D& p, const Point2D& q, const Point2D& r) {
    return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
}

// Funzione per calcolare il contorno convesso
std::vector<Point2D> Polygon2D::compute_convex_hull() const {
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

} // namespace domus::drawing