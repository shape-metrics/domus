#include "domus/planarity/drawing.hpp"

#include <climits>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/drawing/polygon.hpp"
#include "domus/drawing/svg_drawer.hpp"

namespace domus::planarity {
using domus::graph::Attributes;
using domus::graph::Graph;
using namespace domus::drawing;

std::expected<void, std::string>
make_svg(const Graph& graph, const Attributes& attributes, std::filesystem::path path) {
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    for (const size_t node_id : graph.get_nodes_ids()) {
        max_x = std::max(max_x, attributes.get_position_x(node_id));
        max_y = std::max(max_y, attributes.get_position_y(node_id));
    }
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    for (const size_t node_id : graph.get_nodes_ids()) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    }
    const int width = max_x - min_x;
    const int height = max_y - min_y;
    SvgDrawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x - 100, max_x + 100, 0, width);
    auto scale_y = ScaleLinear(min_y - 100, max_y + 100, 0, height);
    std::vector<std::unique_ptr<Point2D>> points;
    points.resize(graph.get_number_of_nodes());
    for (const size_t node_id : graph.get_nodes_ids()) {
        const double x = scale_x.map(attributes.get_position_x(node_id));
        const double y = scale_y.map(attributes.get_position_y(node_id));
        while (points.size() <= node_id) {
            points.push_back(nullptr);
        }
        points[node_id].reset(new Point2D(x, y));
    }
    for (const graph::EdgeId edge : graph.get_all_edges()) {
        Line2D line(*points.at(edge.edge.from_id), *points.at(edge.edge.to_id));
        drawer.add(line);
    }
    for (const size_t node_id : graph.get_nodes_ids()) {
        Square2D square{*points.at(node_id), 20};
        square.setColor("cornflowerblue");
        square.setLabel(std::to_string(node_id));
        drawer.add(square, 4);
    }
    return drawer.save_to_file(path);
}

} // namespace domus::planarity
