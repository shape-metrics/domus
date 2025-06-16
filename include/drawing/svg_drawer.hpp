#ifndef MY_SVG_DRAWER_H
#define MY_SVG_DRAWER_H

#include <sstream>

#include "drawing/linear_scale.hpp"
#include "drawing/polygon.hpp"

class SvgDrawer {
 private:
  std::stringstream m_svg;
  int m_width, m_height;
  ScaleLinear m_scale_y;

 public:
  SvgDrawer(int width, int height);
  void add(Square2D& square);
  void add(Circle2D& circle);
  void add(Line2D& line, std::string color = "black");
  void add(Polygon2D& polygon, std::string color = "black");
  void add(Path2D& path, std::string color = "black");
  void add_and_smooth(Path2D& path, std::string color = "black");
  void save_to_file(const std::string& filename);
};

#endif