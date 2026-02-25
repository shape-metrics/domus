#include "domus/orthogonal/drawing.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "domus/core/graph/graph_utilities.hpp"
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
    vector<int> nodes;
    graph.get_nodes_ids().for_each([&nodes](int node_id) { nodes.push_back(node_id); });
    data["nodes"] = nodes;
    vector<pair<int, int>> edges;
    graph.get_nodes_ids().for_each([&graph, &edges](int node_id) {
        graph.get_neighbors_of_node(node_id).for_each([&edges, node_id](int neighbor_id) {
            if (neighbor_id < node_id)
                return;
            edges.push_back({node_id, neighbor_id});
        });
    });
    data["edges"] = edges;
    const GraphAttributes& attributes = result.attributes;

    graph.get_nodes_ids().for_each([&data, &attributes](int node_id) {
        string s_id = std::to_string(node_id);
        data["node_colors"][s_id] = color_to_string(attributes.get_node_color(node_id));
        data["node_positions"][s_id] = {
            attributes.get_position_x(node_id),
            attributes.get_position_y(node_id)
        };
    });
    json shape_array = json::array();
    optional<string> error_msg;
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (error_msg.has_value())
            return;
        graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            if (error_msg.has_value())
                return;
            if (neighbor_id < node_id)
                return;
            const auto direction = result.shape.get_direction(node_id, neighbor_id);
            if (!direction) {
                string error = "Error in save_orthogonal_drawing_to_file: ";
                error += "direction not set for edge (";
                error += std::to_string(node_id);
                error += ", ";
                error += std::to_string(neighbor_id);
                error += ")";
                error_msg = error;
                return;
            }
            shape_array.push_back(
                {{"u", node_id}, {"v", neighbor_id}, {"dir", direction_to_string(*direction)}}
            );
        });
    });
    data["shape"] = shape_array;
    std::ofstream file(path);
    if (file.is_open()) {
        file << data.dump(4);
        return {};
    }
    string error = "Error in save_orthogonal_drawing_to_file: could not open file ";
    error += path.string();
    return std::unexpected(error);
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
        Direction direction = string_to_direction(item.at("dir"));
        result.shape.set_direction(item.at("u"), item.at("v"), direction);
    }
    return result;
}

expected<void, string>
make_svg(const UndirectedGraph& graph, const GraphAttributes& attributes, path path) {
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    graph.get_nodes_ids().for_each([&](int node_id) {
        max_x = std::max(max_x, attributes.get_position_x(node_id));
        max_y = std::max(max_y, attributes.get_position_y(node_id));
    });
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    graph.get_nodes_ids().for_each([&](int node_id) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    });
    size_t nodes_size = 25;
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (graph.get_degree_of_node(node_id) <= 4)
            return;
        const size_t side = 25 + 3 * (graph.get_degree_of_node(node_id) - 4);
        if (side > nodes_size)
            nodes_size = side;
    });
    const int width = max_x - min_x;
    const int height = max_y - min_y;
    SvgDrawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x - 100, max_x + 100, 0, width);
    auto scale_y = ScaleLinear(min_y - 100, max_y + 100, 0, height);
    unordered_map<int, Point2D> points;
    graph.get_nodes_ids().for_each([&](int node_id) {
        const double x = scale_x.map(attributes.get_position_x(node_id));
        const double y = scale_y.map(attributes.get_position_y(node_id));
        points.emplace(node_id, Point2D(x, y));
    });
    graph.get_nodes_ids().for_each([&](int node_id) {
        graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            Line2D line(points.at(node_id), points.at(neighbor_id));
            drawer.add(line);
        });
    });
    graph.get_nodes_ids().for_each([&](int node_id) {
        const Color color = attributes.get_node_color(node_id);
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
    });
    return drawer.save_to_file(path);
}

int min_coordinate(unordered_map<int, NodesContainer>& coordinate_to_nodes) {
    int min_c = INT_MAX;
    for (const int coord : coordinate_to_nodes | std::views::keys)
        if (coord < min_c)
            min_c = coord;
    return min_c;
}

pair<Int_ToInt_HashMap, Int_ToInt_HashMap>
compute_node_to_index_position(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    constexpr int THRESHOLD = 45;
    unordered_map<int, NodesContainer> coordinate_y_to_nodes;
    graph.get_nodes_ids().for_each([&](int node_id) {
        const int y = attributes.get_position_y(node_id);
        coordinate_y_to_nodes[y].add_node(node_id);
    });
    unordered_map<int, NodesContainer> coordinate_x_to_nodes;
    graph.get_nodes_ids().for_each([&](int node_id) {
        const int x = attributes.get_position_x(node_id);
        coordinate_x_to_nodes[x].add_node(node_id);
    });
    int y_index = 0;
    Int_ToInt_HashMap node_to_coordinate_y;
    int min_y = min_coordinate(coordinate_y_to_nodes);
    while (true) {
        coordinate_y_to_nodes[min_y].for_each([&node_to_coordinate_y, y_index](int node_id) {
            node_to_coordinate_y.add(node_id, y_index);
        });
        coordinate_y_to_nodes.erase(min_y);
        if (coordinate_y_to_nodes.empty())
            break;
        const int next_min_y = min_coordinate(coordinate_y_to_nodes);
        if (next_min_y - min_y >= THRESHOLD)
            ++y_index;
        min_y = next_min_y;
    }
    int x_index = 0;
    Int_ToInt_HashMap node_to_coordinate_x;
    int min_x = min_coordinate(coordinate_x_to_nodes);
    while (true) {
        coordinate_x_to_nodes[min_x].for_each([&node_to_coordinate_x, x_index](int node_id) {
            node_to_coordinate_x.add(node_id, x_index);
        });
        coordinate_x_to_nodes.erase(min_x);
        if (coordinate_x_to_nodes.empty())
            break;
        const int next_min_x = min_coordinate(coordinate_x_to_nodes);
        if (next_min_x - min_x >= THRESHOLD)
            ++x_index;
        min_x = next_min_x;
    }
    return make_pair(std::move(node_to_coordinate_x), std::move(node_to_coordinate_y));
}
