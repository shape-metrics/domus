#include "drawing/polygon.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>

#include "core/graph/graph.hpp"

Point2D::Point2D(double x, double y) : x(x), y(y) {}

double Point2D::distance(const Point2D& other) {
  return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y));
}

Line2D::Line2D(Point2D& p1, Point2D& p2) : p1(p1), p2(p2) {}

bool Line2D::isPointOnLine(Point2D& p) {
  double crossProduct =
      (p.y - p1.y) * (p2.x - p1.x) - (p.x - p1.x) * (p2.y - p1.y);
  if (std::abs(crossProduct) > 1e-7) return false;  // Not on the line
  double dotProduct =
      (p.x - p1.x) * (p2.x - p1.x) + (p.y - p1.y) * (p2.y - p1.y);
  if (dotProduct < 0) return false;  // Not on the segment
  double squaredLength =
      (p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y);
  if (dotProduct > squaredLength) return false;  // Not on the segment
  return true;
}

bool Line2D::isIntersecting(Line2D& l) {
  double denominator =
      (p2.y - p1.y) * (l.p2.x - l.p1.x) - (p2.x - p1.x) * (l.p2.y - l.p1.y);
  if (denominator == 0) return false;  // Lines are parallel
  double ua =
      ((p2.x - p1.x) * (l.p1.y - p1.y) - (p2.y - p1.y) * (l.p1.x - p1.x)) /
      denominator;
  double ub = ((l.p2.x - l.p1.x) * (l.p1.y - p1.y) -
               (l.p2.y - l.p1.y) * (l.p1.x - p1.x)) /
              denominator;
  return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}

Polygon2D::Polygon2D(std::vector<Point2D>& points) {
  assert(points.size() >= 3);
  for (int i = 0; i < points.size(); i++) {
    Point2D& p = points[i];
    m_points.push_back(Point2D(p.x, p.y));
  }
}

std::vector<Point2D>& Polygon2D::getPoints() { return m_points; }

bool Polygon2D::isOnBoundary(Point2D& p) {
  int n = m_points.size();
  for (int i = 0; i < n; i++) {
    Point2D& p1 = m_points[i];
    Point2D& p2 = m_points[(i + 1) % n];
    if (p1.x == p2.x) {
      if (p.x == p1.x && p.y >= std::min(p1.y, p2.y) &&
          p.y <= std::max(p1.y, p2.y))
        return true;
    } else {
      double m = (p2.y - p1.y) / (p2.x - p1.x);
      double c = p1.y - m * p1.x;
      double y = m * p.x + c;
      if (p.y == y && p.x >= std::min(p1.x, p2.x) &&
          p.x <= std::max(p1.x, p2.x))
        return true;
    }
  }
  return false;
}

bool Polygon2D::isInside(Point2D& p) {
  int n = m_points.size();
  int count = 0;
  for (int i = 0; i < n; i++) {
    Point2D& p1 = m_points[i];
    if (p == p1) return true;
    Point2D& p2 = m_points[(i + 1) % n];
    if (p1.y == p2.y) continue;
    if (p.y < std::min(p1.y, p2.y)) continue;
    if (p.y >= std::max(p1.y, p2.y)) continue;
    double x = (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
    if (x > p.x) count++;
  }
  return count % 2 == 1;
}

bool Polygon2D::isInside(Line2D& l) {
  if (!isInside(l.p1) || !isInside(l.p2)) return false;
  int n = m_points.size();
  for (int i = 0; i < n; i++) {
    Point2D& p1 = m_points[i];
    Point2D& p2 = m_points[(i + 1) % n];
    Line2D edge(p1, p2);
    if (l.isIntersecting(edge)) {
      if (l.isPointOnLine(p1) || l.isPointOnLine(p2))
        continue;
      else
        return false;
    }
  }
  if (isOnBoundary(l.p1) && isOnBoundary(l.p2)) {
    auto m = (l.p1 + l.p2) / 2.0;
    if (!isInside(m)) return false;
  }
  return true;
}

// Calcola il determinante tra tre punti (p, q, r)
// Se > 0, indica una svolta a sinistra (convesso);
// se < 0, indica una svolta a destra (concavo);
// se == 0, sono collineari
int cross(const Point2D& p, const Point2D& q, const Point2D& r) {
  return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
}

// Funzione per calcolare il contorno convesso
std::vector<Point2D> Polygon2D::computeConvexHull() {
  int n = m_points.size();
  std::vector<Point2D> points(m_points);
  // Ordina i punti in base all'ordinamento lessicografico
  std::sort(points.begin(), points.end());
  std::vector<Point2D> hull;
  // Costruzione della metà inferiore del contorno
  for (const auto& p : points) {
    while (hull.size() >= 2 &&
           cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
      hull.pop_back();
    hull.push_back(p);
  }
  // Costruzione della metà superiore del contorno
  int lowerSize = hull.size();
  for (int i = n - 1; i >= 0; --i) {
    while (hull.size() > lowerSize &&
           cross(hull[hull.size() - 2], hull[hull.size() - 1], points[i]) <= 0)
      hull.pop_back();
    hull.push_back(points[i]);
  }
  // Rimuove l'ultimo punto poiché è uguale al primo
  hull.pop_back();
  return hull;
}