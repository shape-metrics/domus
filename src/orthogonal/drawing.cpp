#include "domus/orthogonal/drawing.hpp"

#include <cstddef>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/drawing/polygon.hpp"
#include "domus/drawing/svg_drawer.hpp"

#include "../nlohmann/json.hpp"
#include "domus/orthogonal/shape/direction.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::orthogonal {

using json = nlohmann::json;
using namespace domus::graph;
using namespace domus::drawing;
using shape::Direction;

std::expected<void, std::string>
save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, std::filesystem::path path) {
    json data;
    const Graph& graph = result.augmented_graph;
    data["nodes"] = graph.get_nodes_ids() | std::ranges::to<std::vector<size_t>>();
    data["edges"] = graph.get_all_edges() | std::views::transform([](const EdgeId edge) {
                        return std::make_pair(edge.edge.from_id, edge.edge.to_id);
                    }) |
                    std::ranges::to<std::vector<std::pair<size_t, size_t>>>();
    const Attributes& attributes = result.attributes;

    for (const size_t node_id : graph.get_nodes_ids()) {
        std::string s_id = std::to_string(node_id);
        data["node_colors"][s_id] = color_to_string(attributes.get_node_color(node_id));
        data["node_positions"][s_id] = {
            attributes.get_position_x(node_id),
            attributes.get_position_y(node_id)
        };
    }
    json shape_array = json::array();
    for (const EdgeId edge : graph.get_all_edges()) {
        Direction direction = result.shape.get_direction(edge.id);
        shape_array.push_back({{"edge_id", edge.id}, {"dir", direction_to_string(direction)}});
    }
    data["shape"] = shape_array;
    std::ofstream file(path);
    if (file.is_open()) {
        file << data.dump(4);
        return {};
    }
    return std::unexpected(
        std::format("save_orthogonal_drawing_to_file: could not open file {}", path.string())
    );
}

std::expected<OrthogonalDrawing, std::string>
load_orthogonal_drawing_from_file(std::filesystem::path path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(
            std::format("load_orthogonal_drawing_from_file: could not open file {}", path.string())
        );
    }
    json data;
    file >> data;
    OrthogonalDrawing result;
    for (size_t i = 0; i < data.at("nodes").size(); ++i)
        result.augmented_graph.add_node();
    for (size_t node_id : data.at("nodes"))
        if (!result.augmented_graph.has_node(node_id))
            return std::unexpected("load_orthogonal_drawing_from_file: invalid graph");
    for (const auto& edge_arr : data.at("edges"))
        result.augmented_graph.add_edge(edge_arr[0], edge_arr[1]);
    result.attributes.add_attribute(graph::Attribute::NODES_COLOR);
    for (auto& [id_str, color_str] : data.at("node_colors").items()) {
        std::string color = std::string(color_str);
        size_t id = static_cast<size_t>(std::stoi(id_str));
        result.attributes.set_node_color(id, string_to_color(color));
    }
    result.attributes.add_attribute(graph::Attribute::NODES_POSITION);
    for (auto& [id_str, pos_arr] : data.at("node_positions").items()) {
        size_t id = static_cast<size_t>(std::stoi(id_str));
        result.attributes.set_position(id, pos_arr[0], pos_arr[1]);
    }
    for (const auto& item : data.at("shape")) {
        Direction direction = shape::string_to_direction(item.at("dir"));
        result.shape.set_direction(item.at("edge_id"), direction);
    }
    return result;
}

size_t direction_to_index(Direction direction) {
    switch (direction) {
    case Direction::UP:
        return 0;
    case Direction::RIGHT:
        return 1;
    case Direction::DOWN:
        return 2;
    case Direction::LEFT:
        return 3;
    default:
        return 4;
    }
}

inline std::pair<size_t, size_t>
get_other_edge_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    DOMUS_ASSERT(
        graph.get_degree_of_node(node_id) == 2,
        "get_other_neighbor_id: function only for degree 2 nodes"
    );
    std::optional<size_t> other;
    std::optional<size_t> other_edge_id;
    for (const EdgeIter edge : graph.get_edges(node_id))
        if (edge.neighbor_id != neighbor_id) {
            other = edge.neighbor_id;
            other_edge_id = edge.id;
            break;
        }
    DOMUS_ASSERT(
        other.has_value(),
        "get_other_neighbor_id: internal error happened, no other neighbor found for node"
    );
    return {*other, *other_edge_id};
}

inline size_t get_other_neighbor_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    return get_other_edge_id(graph, node_id, neighbor_id).first;
}

std::array<size_t, 4> nodes_at_direction(
    const Graph& graph, size_t node_id, const shape::Shape& shape, const Attributes& attributes
) {
    std::array<size_t, 4> result{0, 0, 0, 0};
    for (auto [edge_id, neighbor_id] : graph.get_edges(node_id)) {
        Color color = attributes.get_node_color(neighbor_id);
        if (color == Color::RED || color == Color::BLACK) {
            size_t index =
                direction_to_index(shape.get_direction(graph, edge_id, node_id, neighbor_id));
            result[index] = result[index] + 1;
            continue;
        }
        auto [other_id, other_edge_id] = get_other_edge_id(graph, neighbor_id, node_id);
        size_t index =
            direction_to_index(shape.get_direction(graph, other_edge_id, neighbor_id, other_id));
        result[index] = result[index] + 1;
    }
    return result;
}

double
compute_side_length(const Graph& graph, const shape::Shape& shape, const Attributes& attributes) {
    size_t max_per_side = 0;
    for (size_t node_id : graph.get_nodes_ids()) {
        auto n_per_direction = nodes_at_direction(graph, node_id, shape, attributes);
        size_t max = *std::max_element(n_per_direction.begin(), n_per_direction.end());
        max_per_side = std::max(max_per_side, max);
    }
    return 20.0 + 6.0 * static_cast<double>(max_per_side);
}

std::expected<void, std::string> make_svg(
    const Graph& graph,
    const Attributes& attributes,
    const shape::Shape& shape,
    std::filesystem::path path
) {
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
    for (const EdgeId edge : graph.get_all_edges()) {
        Line2D line(*points.at(edge.edge.from_id), *points.at(edge.edge.to_id));
        drawer.add(line);
    }
    const double side = compute_side_length(graph, shape, attributes);
    for (const size_t node_id : graph.get_nodes_ids()) {
        Color color = attributes.get_node_color(node_id);
        if (color == Color::RED)
            continue;
        if (color == Color::GREEN)
            continue;
        if (color == Color::BLUE)
            continue;
        if (color == Color::RED_SPECIAL)
            continue;
        if (color == Color::BLUE_DARK)
            continue;
        if (color == Color::GREEN_DARK)
            continue;
        Square2D square{*points.at(node_id), side};
        square.setColor(color_to_string(color));
        square.setColor("cornflowerblue");
        square.setLabel(std::to_string(node_id));
        drawer.add(square, side / 4);
    }
    return drawer.save_to_file(path);
}

int min_coordinate(std::map<int, std::vector<size_t>>& coordinate_to_nodes) {
    return coordinate_to_nodes.begin()->first;
}

std::pair<std::vector<size_t>, std::vector<size_t>>
compute_node_to_index_position(const Graph& graph, const Attributes& attributes) {
    constexpr int THRESHOLD = 45;
    std::map<int, std::vector<size_t>> coordinate_y_to_nodes;
    for (const size_t node_id : graph.get_nodes_ids()) {
        int y = attributes.get_position_y(node_id);
        coordinate_y_to_nodes[y].push_back(node_id);
    }
    std::map<int, std::vector<size_t>> coordinate_x_to_nodes;
    for (const size_t node_id : graph.get_nodes_ids()) {
        int x = attributes.get_position_x(node_id);
        coordinate_x_to_nodes[x].push_back(node_id);
    }
    size_t y_index = 0;
    std::vector<std::optional<size_t>> node_to_coordinate_y(
        graph.get_number_of_nodes(),
        std::nullopt
    );
    int min_y = min_coordinate(coordinate_y_to_nodes);
    while (true) {
        for (size_t node_id : coordinate_y_to_nodes[min_y])
            node_to_coordinate_y[node_id] = y_index;
        coordinate_y_to_nodes.erase(min_y);
        if (coordinate_y_to_nodes.empty())
            break;
        int next_min_y = min_coordinate(coordinate_y_to_nodes);
        if (next_min_y - min_y >= THRESHOLD)
            ++y_index;
        min_y = next_min_y;
    }
    size_t x_index = 0;
    std::vector<std::optional<size_t>> node_to_coordinate_x(
        graph.get_number_of_nodes(),
        std::nullopt
    );
    int min_x = min_coordinate(coordinate_x_to_nodes);
    while (true) {
        for (size_t node_id : coordinate_x_to_nodes[min_x])
            node_to_coordinate_x[node_id] = x_index;
        coordinate_x_to_nodes.erase(min_x);
        if (coordinate_x_to_nodes.empty())
            break;
        int next_min_x = min_coordinate(coordinate_x_to_nodes);
        if (next_min_x - min_x >= THRESHOLD)
            ++x_index;
        min_x = next_min_x;
    }

    return std::make_pair(
        node_to_coordinate_x | std::views::transform([](std::optional<size_t> node_id) {
            return node_id.value();
        }) | std::ranges::to<std::vector<size_t>>(),
        node_to_coordinate_y | std::views::transform([](std::optional<size_t> node_id) {
            return node_id.value();
        }) | std::ranges::to<std::vector<size_t>>()
    );
}

} // namespace domus::orthogonal