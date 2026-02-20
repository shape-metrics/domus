#include "domus/orthogonal/drawing_stats.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stdlib.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/utils.hpp"
#include "domus/orthogonal/drawing.hpp"

using namespace std;

vector<int> compute_edge_lengths(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    vector<int> edge_lengths;
    unordered_set<int> visited;
    for (const int node_id : graph.get_nodes_ids()) {
        if (attributes.get_node_color(node_id) != Color::BLACK)
            continue;
        function<void(int, int, int)> dfs =
            [&](const int current_id, const int black_id, const int current_length) {
                visited.insert(current_id);
                for (int neighbor_id : graph.get_neighbors_of_node(current_id)) {
                    if (visited.contains(neighbor_id))
                        continue;
                    const int x1 = node_to_coordinate_x[current_id];
                    const int y1 = node_to_coordinate_y[current_id];
                    const int x2 = node_to_coordinate_x[neighbor_id];
                    const int y2 = node_to_coordinate_y[neighbor_id];
                    const int length = abs(x1 - x2) + abs(y1 - y2);
                    const Color neighbor_color = attributes.get_node_color(neighbor_id);
                    if (neighbor_color != Color::BLACK)
                        dfs(neighbor_id, black_id, current_length + length);
                    else {
                        if (black_id < neighbor_id) {
                            int total_length = current_length + length;
                            edge_lengths.push_back(total_length);
                        }
                    }
                }
                visited.erase(current_id);
            };
        dfs(node_id, node_id, 0);
    }
    return edge_lengths;
}

int compute_total_edge_length(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const vector<int> edge_lengths = compute_edge_lengths(graph, result.attributes);
    int total_edge_length = 0;
    for (const int length : edge_lengths)
        total_edge_length += length;
    return total_edge_length;
}

int compute_max_edge_length(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<int> edge_lengths = compute_edge_lengths(graph, attributes);
    int max_edge_length = 0;
    for (const int length : edge_lengths)
        if (length > max_edge_length)
            max_edge_length = length;
    return max_edge_length;
}

double compute_edge_length_std_dev(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<int> edge_lengths = compute_edge_lengths(graph, attributes);
    return compute_stddev(edge_lengths);
}

vector<int> compute_bends_counts(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    vector<int> bends_counts;
    for (int node_id : graph.get_nodes_ids()) {
        if (attributes.get_node_color(node_id) != Color::BLACK)
            continue;
        unordered_set<int> visited;
        function<void(int, int, int, int)> dfs =
            [&](int current, int black, int count, int previous_id) {
                visited.insert(current);
                for (int neighbor_id : graph.get_neighbors_of_node(current)) {
                    if (visited.contains(neighbor_id))
                        continue;
                    const Color neighbor_color = attributes.get_node_color(neighbor_id);
                    if (neighbor_color != Color::BLACK) {
                        if (node_to_coordinate_x[previous_id] ==
                                node_to_coordinate_x[neighbor_id] &&
                            node_to_coordinate_y[previous_id] == node_to_coordinate_y[neighbor_id])
                            dfs(neighbor_id, black, count, current);
                        else
                            dfs(neighbor_id, black, count + 1, current);
                    } else if (black < neighbor_id) {
                        if (node_to_coordinate_x[current] == node_to_coordinate_x[neighbor_id] &&
                            node_to_coordinate_y[current] == node_to_coordinate_y[neighbor_id])
                            count--;
                        bends_counts.push_back(count);
                    }
                }
                visited.erase(current);
            };
        dfs(node_id, node_id, 0, node_id);
    }
    return bends_counts;
}

int compute_total_bends(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<int> bends_counts = compute_bends_counts(graph, attributes);
    int total_bends = 0;
    for (const int count : bends_counts)
        total_bends += count;
    return total_bends;
}

int compute_max_bends_per_edge(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<int> bends_counts = compute_bends_counts(graph, attributes);
    int max_bends = 0;
    for (const int count : bends_counts)
        if (count > max_bends)
            max_bends = count;
    return max_bends;
}

double compute_bends_std_dev(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const auto& attributes = result.attributes;
    return compute_stddev(compute_bends_counts(graph, attributes));
}

int compute_total_area(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, result.attributes);
    int max_x = -INT_MAX;
    int max_y = -INT_MAX;
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    for (const int node_id : graph.get_nodes_ids()) {
        const int x = node_to_coordinate_x[node_id];
        const int y = node_to_coordinate_y[node_id];
        max_x = max(max_x, x);
        max_y = max(max_y, y);
        min_x = min(min_x, x);
        min_y = min(min_y, y);
    }
    return (max_x - min_x + 1) * (max_y - min_y + 1);
}

bool do_edges_cross(
    int i,
    int j,
    int k,
    int l,
    const unordered_map<int, int>& node_to_coordinate_x,
    const unordered_map<int, int>& node_to_coordinate_y
) {
    int i_pos_x = node_to_coordinate_x.at(i);
    int i_pos_y = node_to_coordinate_y.at(i);
    int j_pos_x = node_to_coordinate_x.at(j);
    int j_pos_y = node_to_coordinate_y.at(j);
    int k_pos_x = node_to_coordinate_x.at(k);
    int k_pos_y = node_to_coordinate_y.at(k);
    int l_pos_x = node_to_coordinate_x.at(l);
    int l_pos_y = node_to_coordinate_y.at(l);

    bool is_i_j_horizontal = i_pos_y == j_pos_y;
    bool is_k_l_horizontal = k_pos_y == l_pos_y;

    if (is_i_j_horizontal && is_k_l_horizontal)
        return false;
    if (!is_i_j_horizontal && !is_k_l_horizontal)
        return false;
    if (!is_i_j_horizontal)
        return do_edges_cross(k, l, i, j, node_to_coordinate_x, node_to_coordinate_x);
    if (i_pos_x == k_pos_x || i_pos_x == l_pos_x || j_pos_x == k_pos_x || j_pos_x == l_pos_x ||
        i_pos_y == k_pos_y || i_pos_y == l_pos_y || j_pos_y == k_pos_y || j_pos_y == l_pos_y)
        return false;
    if (k_pos_x < min(i_pos_x, j_pos_x) || k_pos_x > max(i_pos_x, j_pos_x))
        return false;
    if (i_pos_y < min(k_pos_y, l_pos_y) || i_pos_y > max(k_pos_y, l_pos_y))
        return false;
    return true;
}

bool do_edges_cross(
    const GraphAttributes& attributes, const int i, const int j, const int k, const int l
) {
    int i_pos_x = attributes.get_position_x(i);
    int i_pos_y = attributes.get_position_y(i);
    int j_pos_x = attributes.get_position_x(j);
    int j_pos_y = attributes.get_position_y(j);
    int k_pos_x = attributes.get_position_x(k);
    int k_pos_y = attributes.get_position_y(k);
    int l_pos_x = attributes.get_position_x(l);
    int l_pos_y = attributes.get_position_y(l);

    if (abs(i_pos_x - k_pos_x) < 0.2 || abs(i_pos_x - l_pos_x) < 0.2 ||
        abs(i_pos_y - k_pos_y) < 0.2 || abs(i_pos_y - l_pos_y) < 0.2 ||
        abs(j_pos_x - k_pos_x) < 0.2 || abs(j_pos_x - l_pos_x) < 0.2 ||
        abs(j_pos_y - k_pos_y) < 0.2 || abs(j_pos_y - l_pos_y) < 0.2)
        return false;

    bool is_i_j_horizontal = i_pos_y == j_pos_y;
    bool is_k_l_horizontal = k_pos_y == l_pos_y;

    if (is_i_j_horizontal && is_k_l_horizontal) {
        return (i_pos_y == k_pos_y) && ((i_pos_x <= k_pos_x && j_pos_x >= k_pos_x) ||
                                        (i_pos_x <= l_pos_x && j_pos_x >= l_pos_x) ||
                                        (j_pos_x <= k_pos_x && i_pos_x >= k_pos_x) ||
                                        (j_pos_x <= l_pos_x && i_pos_x >= l_pos_x));
    }
    if (!is_i_j_horizontal && !is_k_l_horizontal) {
        return (i_pos_x == k_pos_x) && ((i_pos_y <= k_pos_y && j_pos_y >= k_pos_y) ||
                                        (i_pos_y <= l_pos_y && j_pos_y >= l_pos_y) ||
                                        (j_pos_y <= k_pos_y && i_pos_y >= k_pos_y) ||
                                        (j_pos_y <= l_pos_y && i_pos_y >= l_pos_y));
    }
    if (!is_i_j_horizontal)
        return do_edges_cross(attributes, k, l, i, j);
    if (k_pos_x < min(i_pos_x, j_pos_x) || k_pos_x > max(i_pos_x, j_pos_x))
        return false;
    if (i_pos_y < min(k_pos_y, l_pos_y) || i_pos_y > max(k_pos_y, l_pos_y))
        return false;
    return true;
}

int compute_total_crossings(const OrthogonalDrawing& result) {
    const UndirectedGraph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    int total_crossings = 0;
    vector<pair<int, int>> edges;
    for (int node_id : graph.get_nodes_ids())
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (neighbor_id < node_id)
                continue;
            edges.emplace_back(node_id, neighbor_id);
        }
    for (size_t i = 0; i < edges.size(); ++i) {
        int node_1_id = edges[i].first;
        int node_2_id = edges[i].second;
        for (size_t j = i + 1; j < edges.size(); ++j) {
            int node_3_id = edges[j].first;
            int node_4_id = edges[j].second;
            if (node_1_id == node_3_id || node_1_id == node_4_id || node_2_id == node_3_id ||
                node_2_id == node_4_id)
                continue;
            if (do_edges_cross(attributes, node_1_id, node_2_id, node_3_id, node_4_id))
                ++total_crossings;
        }
    }
    return total_crossings;
}

OrthogonalStats compute_all_orthogonal_stats(const OrthogonalDrawing& result) {
    return {
        compute_total_crossings(result),
        compute_total_bends(result),
        compute_total_area(result),
        compute_total_edge_length(result),
        compute_max_edge_length(result),
        compute_edge_length_std_dev(result),
        compute_max_bends_per_edge(result),
        compute_bends_std_dev(result)
    };
}

string orthogonal_stats_to_string(const OrthogonalStats& stats) {
    stringstream ss;
    ss << "Area: " << stats.area << "\n"
       << "Crossings: " << stats.crossings << "\n"
       << "Bends: " << stats.bends << "\n"
       << "Total edge length: " << stats.total_edge_length << "\n"
       << "Max edge length: " << stats.max_edge_length << "\n"
       << "Edge length stddev: " << stats.edge_length_stddev << "\n"
       << "Max bends per edge: " << stats.max_bends_per_edge << "\n"
       << "Bends stddev: " << stats.bends_stddev << "\n";
    return ss.str();
}

void print_orthogonal_stats(const OrthogonalStats& stats) {
    std::cout << orthogonal_stats_to_string(stats);
}