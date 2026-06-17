#pragma once

#include <expected>
#include <filesystem>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "domus/drawing/draw_elements.hpp"
#include "domus/drawing/linear_scale.hpp"

namespace domus::drawing {

using DrawElement = std::
    variant<Text2D, Square2D, RoundSquare2D, Circle2D, Line2D, Polygon2D, Path2D, SmoothPath2D>;

class Drawer {
    std::vector<DrawElement> m_elements;
    double m_width, m_height;
    ScaleLinear m_scale_y;

    void render_to_stream(std::ostream& os) const;

  public:
    Drawer(double width, double height);
    void add(const Text2D& text);
    void add(const Square2D& square);
    void add(const RoundSquare2D& square);
    void add(const Circle2D& circle);
    void add(const Line2D& line);
    void add(const Polygon2D& polygon);
    void add(const Path2D& path);
    void add(const SmoothPath2D& path);
    const std::vector<DrawElement>& get_elements() const;

    std::string to_string() const;
    std::expected<void, std::string> save_to_file(std::filesystem::path path) const;
};

} // namespace domus::drawing