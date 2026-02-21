#include "domus/orthogonal/drawing.hpp"

#include <algorithm>
#include <fstream>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "domus/core/utils.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/drawing/polygon.hpp"
#include "domus/drawing/svg_drawer.hpp"
#include "domus/nlohmann/json.hpp"

using json = nlohmann::json;

using namespace std;
using namespace std::filesystem;

expected<void, string> save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, path path) {
    json data;
    const UndirectedGraph& graph = result.augmented_graph;
    data["nodes"] = graph.get_nodes_ids();
    vector<pair<int, int>> edges;
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (neighbor_id < node_id)
                continue;
            edges.push_back({node_id, neighbor_id});
        }
    }
    data["edges"] = edges;
    const GraphAttributes& attributes = result.attributes;
    for (int node_id : graph.get_nodes_ids()) {
        string s_id = std::to_string(node_id);
        data["node_colors"][s_id] = color_to_string(attributes.get_node_color(node_id));
        data["node_positions"][s_id] = {
            attributes.get_position_x(node_id),
            attributes.get_position_y(node_id)
        };
    }
    json shape_array = json::array();

    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (neighbor_id < node_id)
                continue;
            const auto direction = result.shape.get_direction(node_id, neighbor_id);
            if (!direction) {
                string error_msg = "Error in save_orthogonal_drawing_to_file: ";
                error_msg += "direction not set for edge (";
                error_msg += std::to_string(node_id);
                error_msg += ", ";
                error_msg += std::to_string(neighbor_id);
                error_msg += ")";
                return std::unexpected(error_msg);
            }
            shape_array.push_back(
                {{"u", node_id}, {"v", neighbor_id}, {"dir", direction_to_string(*direction)}}
            );
        }
    }
    data["shape"] = shape_array;
    std::ofstream file(path);
    if (file.is_open()) {
        file << data.dump(4);
        return {};
    } else {
        string error_msg = "Error in save_orthogonal_drawing_to_file: could not open file ";
        error_msg += path.string();
        return std::unexpected(error_msg);
    }
}

expected<OrthogonalDrawing, string> load_orthogonal_drawing_from_file(path path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        string error_msg = "Error in load_orthogonal_drawing_from_file: could not open file ";
        error_msg += path.string();
        return std::unexpected(error_msg);
    }
    json data;
    file >> data;
    OrthogonalDrawing result;
    for (int node_id : data.at("nodes"))
        result.augmented_graph.add_node(node_id);
    for (const auto& edge_arr : data.at("edges"))
        result.augmented_graph.add_edge(edge_arr[0], edge_arr[1]);
    result.attributes.add_attribute(Attribute::NODES_COLOR);
    for (auto& [id_str, color_str] : data.at("node_colors").items())
        result.attributes.set_node_color(std::stoi(id_str), string_to_color(color_str));
    result.attributes.add_attribute(Attribute::NODES_POSITION);
    for (auto& [id_str, pos_arr] : data.at("node_positions").items())
        result.attributes.set_position(std::stoi(id_str), pos_arr[0], pos_arr[1]);
    for (const auto& item : data.at("shape")) {
        auto direction = string_to_direction(item.at("dir"));
        if (!direction) {
            string error_msg = "Error in load_orthogonal_drawing_from_file: ";
            error_msg += direction.error();
            return std::unexpected(error_msg);
        }
        auto res = result.shape.set_direction(item.at("u"), item.at("v"), *direction);
        if (!res) {
            string error_msg = "Error in load_orthogonal_drawing_from_file: ";
            error_msg += res.error();
            return std::unexpected(error_msg);
        }
    }
    return result;
}

expected<void, string>
make_svg(const UndirectedGraph& graph, const GraphAttributes& attributes, path path) {
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    for (int node_id : graph.get_nodes_ids()) {
        max_x = std::max(max_x, attributes.get_position_x(node_id));
        max_y = std::max(max_y, attributes.get_position_y(node_id));
    }
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    for (int node_id : graph.get_nodes_ids()) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    }
    size_t nodes_size = 25;
    for (int node_id : graph.get_nodes_ids()) {
        if (graph.get_degree_of_node(node_id) <= 4)
            continue;
        const size_t side = 25 + 3 * (graph.get_degree_of_node(node_id) - 4);
        if (side > nodes_size)
            nodes_size = side;
    }
    const int width = max_x - min_x;
    const int height = max_y - min_y;
    SvgDrawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x - 100, max_x + 100, 0, width);
    auto scale_y = ScaleLinear(min_y - 100, max_y + 100, 0, height);
    unordered_map<int, Point2D> points;
    for (int node_id : graph.get_nodes_ids()) {
        const double x = scale_x.map(attributes.get_position_x(node_id));
        const double y = scale_y.map(attributes.get_position_y(node_id));
        points.emplace(node_id, Point2D(x, y));
    }
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            Line2D line(points.at(node_id), points.at(neighbor_id));
            drawer.add(line);
        }
    }
    for (int node_id : graph.get_nodes_ids()) {
        const Color color = attributes.get_node_color(node_id);
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
        const size_t side =
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
    }
    return drawer.save_to_file(path);
}

int min_coordinate(unordered_map<int, unordered_set<int>> coordinate_to_nodes) {
    int min_c = INT_MAX;
    for (const int coord : coordinate_to_nodes | std::views::keys)
        if (coord < min_c)
            min_c = coord;
    return min_c;
}

pair<unordered_map<int, int>, unordered_map<int, int>>
compute_node_to_index_position(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    constexpr int THRESHOLD = 45;
    unordered_map<int, unordered_set<int>> coordinate_y_to_nodes;
    for (const int node_id : graph.get_nodes_ids()) {
        const int y = attributes.get_position_y(node_id);
        coordinate_y_to_nodes[y].insert(node_id);
    }
    unordered_map<int, unordered_set<int>> coordinate_x_to_nodes;
    for (const int node_id : graph.get_nodes_ids()) {
        const int x = attributes.get_position_x(node_id);
        coordinate_x_to_nodes[x].insert(node_id);
    }
    int y_index = 0;
    unordered_map<int, int> node_to_coordinate_y;
    int min_y = min_coordinate(coordinate_y_to_nodes);
    while (true) {
        for (const int node_id : coordinate_y_to_nodes[min_y])
            node_to_coordinate_y[node_id] = y_index;
        coordinate_y_to_nodes.erase(min_y);
        if (coordinate_y_to_nodes.empty())
            break;
        const int next_min_y = min_coordinate(coordinate_y_to_nodes);
        if (next_min_y - min_y >= THRESHOLD)
            ++y_index;
        min_y = next_min_y;
    }
    int x_index = 0;
    unordered_map<int, int> node_to_coordinate_x;
    int min_x = min_coordinate(coordinate_x_to_nodes);
    while (true) {
        for (const int node_id : coordinate_x_to_nodes[min_x])
            node_to_coordinate_x[node_id] = x_index;
        coordinate_x_to_nodes.erase(min_x);
        if (coordinate_x_to_nodes.empty())
            break;
        const int next_min_x = min_coordinate(coordinate_x_to_nodes);
        if (next_min_x - min_x >= THRESHOLD)
            ++x_index;
        min_x = next_min_x;
    }
    return make_pair(node_to_coordinate_x, node_to_coordinate_y);
}
