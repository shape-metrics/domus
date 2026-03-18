#include "domus/orthogonal/drawing.hpp"

#include <cstddef>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/drawing/polygon.hpp"
#include "domus/drawing/svg_drawer.hpp"

#include "../nlohmann/json.hpp"

using json = nlohmann::json;

std::expected<void, std::string>
save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, std::filesystem::path path) {
    json data;
    const Graph& graph = result.augmented_graph;
    std::vector<size_t> nodes;
    graph.for_each_node([&nodes](size_t node_id) { nodes.push_back(node_id); });
    data["nodes"] = nodes;
    std::vector<std::pair<size_t, size_t>> edges;
    graph.for_each_node([&graph, &edges](size_t node_id) {
        graph.for_each_neighbor(node_id, [&edges, node_id](size_t neighbor_id) {
            if (neighbor_id < node_id)
                return;
            edges.push_back({node_id, neighbor_id});
        });
    });
    data["edges"] = edges;
    const GraphAttributes& attributes = result.attributes;

    graph.for_each_node([&data, &attributes](size_t node_id) {
        std::string s_id = std::to_string(node_id);
        data["node_colors"][s_id] = color_to_string(attributes.get_node_color(node_id));
        data["node_positions"][s_id] = {
            attributes.get_position_x(node_id),
            attributes.get_position_y(node_id)
        };
    });
    json shape_array = json::array();
    std::optional<std::string> error_msg;
    graph.for_each_node([&](size_t node_id) {
        if (error_msg.has_value())
            return;
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (error_msg.has_value())
                return;
            if (neighbor_id < node_id)
                return;
            Direction direction = result.shape.get_direction(node_id, neighbor_id);
            shape_array.push_back(
                {{"u", node_id}, {"v", neighbor_id}, {"dir", direction_to_string(direction)}}
            );
        });
    });
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
    result.attributes.add_attribute(Attribute::NODES_COLOR);
    for (auto& [id_str, color_str] : data.at("node_colors").items()) {
        std::string color = std::string(color_str);
        size_t id = static_cast<size_t>(std::stoi(id_str));
        result.attributes.set_node_color(id, string_to_color(color));
    }
    result.attributes.add_attribute(Attribute::NODES_POSITION);
    for (auto& [id_str, pos_arr] : data.at("node_positions").items()) {
        size_t id = static_cast<size_t>(std::stoi(id_str));
        result.attributes.set_position(id, pos_arr[0], pos_arr[1]);
    }
    for (const auto& item : data.at("shape")) {
        Direction direction = string_to_direction(item.at("dir"));
        result.shape.set_direction(item.at("u"), item.at("v"), direction);
    }
    return result;
}

std::expected<void, std::string>
make_svg(const Graph& graph, const GraphAttributes& attributes, std::filesystem::path path) {
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    graph.for_each_node([&](size_t node_id) {
        max_x = std::max(max_x, attributes.get_position_x(node_id));
        max_y = std::max(max_y, attributes.get_position_y(node_id));
    });
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    graph.for_each_node([&](size_t node_id) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    });
    const int width = max_x - min_x;
    const int height = max_y - min_y;
    SvgDrawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x - 100, max_x + 100, 0, width);
    auto scale_y = ScaleLinear(min_y - 100, max_y + 100, 0, height);
    std::unordered_map<size_t, Point2D> points;
    graph.for_each_node([&](size_t node_id) {
        const double x = scale_x.map(attributes.get_position_x(node_id));
        const double y = scale_y.map(attributes.get_position_y(node_id));
        points.emplace(node_id, Point2D(x, y));
    });
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            Line2D line(points.at(node_id), points.at(neighbor_id));
            drawer.add(line);
        });
    });
    graph.for_each_node([&](size_t node_id) {
        Color color = attributes.get_node_color(node_id);
        if (color == Color::RED)
            return;
        if (color == Color::GREEN)
            return;
        if (color == Color::BLUE)
            return;
        if (color == Color::RED_SPECIAL)
            return;
        if (color == Color::BLUE_DARK)
            return;
        if (color == Color::GREEN_DARK)
            return;
        size_t side =
            graph.get_degree_of_node(node_id) <= 4
                ? 25
                : static_cast<size_t>(
                      ceil(25 * sqrt(static_cast<double>(graph.get_degree_of_node(node_id) - 3)))
                  );
        Square2D square{points.at(node_id), static_cast<double>(side)};
        square.setColor(color_to_string(color));
        square.setColor("cornflowerblue");
        square.setLabel(std::to_string(node_id));
        drawer.add(square, 5);
    });
    return drawer.save_to_file(path);
}

int min_coordinate(std::map<int, std::vector<size_t>>& coordinate_to_nodes) {
    return coordinate_to_nodes.begin()->first;
}

std::pair<std::vector<size_t>, std::vector<size_t>>
compute_node_to_index_position(const Graph& graph, const GraphAttributes& attributes) {
    constexpr int THRESHOLD = 45;
    std::map<int, std::vector<size_t>> coordinate_y_to_nodes;
    graph.for_each_node([&](size_t node_id) {
        int y = attributes.get_position_y(node_id);
        coordinate_y_to_nodes[y].push_back(node_id);
    });
    std::map<int, std::vector<size_t>> coordinate_x_to_nodes;
    graph.for_each_node([&](size_t node_id) {
        int x = attributes.get_position_x(node_id);
        coordinate_x_to_nodes[x].push_back(node_id);
    });
    size_t y_index = 0;
    std::vector<std::optional<size_t>> node_to_coordinate_y(graph.size(), std::nullopt);
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
    std::vector<std::optional<size_t>> node_to_coordinate_x(graph.size(), std::nullopt);
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
