#include "drawing/svg_drawer.hpp"

#include <fstream>

SvgDrawer::SvgDrawer(int width, int height)
    : m_width(width), m_height(height), m_scale_y(0, height, height, 0) {
  m_svg << "<svg height=\"" << m_height << "\" width=\"" << m_width << "\" ";
  m_svg << "xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
  m_svg << "<rect width=\"" << m_width << "\" height=\"" << m_height << "\" ";
  m_svg << "x=\"0\" y=\"0\" fill=\"white\" />";
}

void SvgDrawer::add(Square2D& square) {
  std::string color;
  if (square.hasColor())
    color = square.getColor();
  else
    color = "black";
  double side = square.getSide();
  double x = square.getCenter().x - side / 2;
  double y = square.getCenter().y + side / 2;
  m_svg << "<rect x=\"" << x << "\" y=\"" << m_scale_y.map(y) << "\" ";
  m_svg << "width=\"" << side << "\" height=\"" << side << "\" fill=\"" << color
        << "\" />" << std::endl;
  if (square.hasLabel()) {
    m_svg << "<text x=\"" << square.getCenter().x - 9 << "\" y=\""
          << m_scale_y.map(square.getCenter().y - 7) << "\" ";
    m_svg << "font-family=\"Verdana\" font-size=\"18\" fill=\"white\">"
          << square.getLabel() << "</text>" << std::endl;
  }
}

void SvgDrawer::add(Line2D& line, std::string color) {
  m_svg << "<line x1=\"" << line.p1.x << "\" y1=\"" << m_scale_y.map(line.p1.y)
        << "\" ";
  m_svg << "x2=\"" << line.p2.x << "\" y2=\"" << m_scale_y.map(line.p2.y)
        << "\" ";
  m_svg << "style=\"stroke:" << color << ";stroke-width:2\" />" << std::endl;
}

void SvgDrawer::add(Polygon2D& polygon, std::string color) {
  m_svg << "<polygon points=\"";
  for (const auto& point : polygon.getPoints())
    m_svg << point.x << "," << m_scale_y.map(point.y) << " ";
  m_svg << "\" style=\"fill:white;stroke:" << color << ";stroke-width:2\" />"
        << std::endl;
}

void SvgDrawer::add(Path2D& path, std::string color) {
  m_svg << "<path d=\"";
  for (int i = 0; i < path.points.size(); i++) {
    if (i == 0)
      m_svg << "M" << path.points[i].x << "," << m_scale_y.map(path.points[i].y)
            << " ";
    else
      m_svg << "L" << path.points[i].x << "," << m_scale_y.map(path.points[i].y)
            << " ";
  }
  m_svg << "\" style=\"fill:none;stroke:" << color << ";stroke-width:1\" />"
        << std::endl;
}

void SvgDrawer::add_and_smooth(Path2D& path, std::string color) {
  m_svg << "<path d=\"";
  for (int i = 0; i < path.points.size(); i++) {
    if (i == 0)
      m_svg << "M" << path.points[i].x << "," << m_scale_y.map(path.points[i].y)
            << " ";
    else
      m_svg << "T" << path.points[i].x << "," << m_scale_y.map(path.points[i].y)
            << " ";
  }
  m_svg << "\" style=\"fill:none;stroke:" << color << ";stroke-width:1\" />"
        << std::endl;
}

void SvgDrawer::save_to_file(const std::string& filename) {
  std::ofstream svgFile(filename);
  svgFile << m_svg.str();
  svgFile << "</svg>" << std::endl;
}