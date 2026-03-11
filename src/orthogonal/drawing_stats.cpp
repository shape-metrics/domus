#include "domus/orthogonal/drawing_stats.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <print>
#include <sstream>
#include <stdlib.h>
#include <utility>
#include <vector>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/utils.hpp"
#include "domus/orthogonal/drawing.hpp"

using namespace std;

vector<size_t> compute_edge_lengths(const Graph& graph, const GraphAttributes& attributes) {
    const auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    vector<size_t> edge_lengths;
    NodesContainer visited;
    graph.for_each_node([&](size_t node_id) {
        if (attributes.get_node_color(node_id) != Color::BLACK)
            return;
        function<void(size_t, size_t, size_t)> dfs =
            [&](size_t current_id, size_t black_id, size_t current_length) {
                visited.add_node(current_id);
                graph.for_each_neighbor(current_id, [&](size_t neighbor_id) {
                    if (visited.has_node(neighbor_id))
                        return;
                    size_t x1 = node_to_coordinate_x.get(current_id);
                    size_t y1 = node_to_coordinate_y.get(current_id);
                    size_t x2 = node_to_coordinate_x.get(current_id);
                    size_t y2 = node_to_coordinate_y.get(current_id);
                    if (x1 < x2) {
                        size_t temp = x1;
                        x1 = x2;
                        x2 = temp;
                    }
                    if (y1 < y2) {
                        size_t temp = y1;
                        y1 = y2;
                        y2 = temp;
                    }
                    size_t length = (x1 - x2) + (y1 - y2);
                    Color neighbor_color = attributes.get_node_color(neighbor_id);
                    if (neighbor_color != Color::BLACK)
                        dfs(neighbor_id, black_id, current_length + length);
                    else {
                        if (black_id < neighbor_id) {
                            size_t total_length = current_length + length;
                            edge_lengths.push_back(total_length);
                        }
                    }
                });
                visited.erase(current_id);
            };
        dfs(node_id, node_id, 0u);
    });
    return edge_lengths;
}

size_t compute_total_edge_length(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const vector<size_t> edge_lengths = compute_edge_lengths(graph, result.attributes);
    size_t total_edge_length = 0;
    for (size_t length : edge_lengths)
        total_edge_length += length;
    return total_edge_length;
}

size_t compute_max_edge_length(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<size_t> edge_lengths = compute_edge_lengths(graph, attributes);
    size_t max_edge_length = 0;
    for (size_t length : edge_lengths)
        if (length > max_edge_length)
            max_edge_length = length;
    return max_edge_length;
}

double compute_edge_length_std_dev(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<size_t> edge_lengths = compute_edge_lengths(graph, attributes);
    return compute_stddev(edge_lengths);
}

vector<size_t> compute_bends_counts(const Graph& graph, const GraphAttributes& attributes) {
    const auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    vector<size_t> bends_counts;
    graph.for_each_node([&](size_t node_id) {
        if (attributes.get_node_color(node_id) != Color::BLACK)
            return;
        NodesContainer visited;
        function<void(size_t, size_t, size_t, size_t)> dfs =
            [&](size_t current, size_t black, size_t count, size_t previous_id) {
                visited.add_node(current);
                graph.for_each_neighbor(current, [&](size_t neighbor_id) {
                    if (visited.has_node(neighbor_id))
                        return;
                    Color neighbor_color = attributes.get_node_color(neighbor_id);
                    if (neighbor_color != Color::BLACK) {
                        if (node_to_coordinate_x.get(previous_id) ==
                                node_to_coordinate_x.get(neighbor_id) &&
                            node_to_coordinate_y.get(previous_id) ==
                                node_to_coordinate_y.get(neighbor_id))
                            dfs(neighbor_id, black, count, current);
                        else
                            dfs(neighbor_id, black, count + 1, current);
                    } else if (black < neighbor_id) {
                        if (node_to_coordinate_x.get(current) ==
                                node_to_coordinate_x.get(neighbor_id) &&
                            node_to_coordinate_y.get(current) ==
                                node_to_coordinate_y.get(neighbor_id))
                            count--;
                        bends_counts.push_back(count);
                    }
                });
                visited.erase(current);
            };
        dfs(node_id, node_id, 0, node_id);
    });
    return bends_counts;
}

size_t compute_total_bends(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<size_t> bends_counts = compute_bends_counts(graph, attributes);
    size_t total_bends = 0;
    for (size_t count : bends_counts)
        total_bends += count;
    return total_bends;
}

size_t compute_max_bends_per_edge(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const vector<size_t> bends_counts = compute_bends_counts(graph, attributes);
    size_t max_bends = 0;
    for (size_t count : bends_counts)
        if (count > max_bends)
            max_bends = count;
    return max_bends;
}

double compute_bends_std_dev(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const auto& attributes = result.attributes;
    return compute_stddev(compute_bends_counts(graph, attributes));
}

size_t compute_total_area(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, result.attributes);
    size_t max_x = 0;
    size_t max_y = 0;
    size_t min_x = INT_MAX;
    size_t min_y = INT_MAX;
    graph.for_each_node([&](size_t node_id) {
        size_t x = node_to_coordinate_x.get(node_id);
        size_t y = node_to_coordinate_y.get(node_id);
        max_x = max(max_x, x);
        max_y = max(max_y, y);
        min_x = min(min_x, x);
        min_y = min(min_y, y);
    });
    return static_cast<size_t>((max_x - min_x + 1) * (max_y - min_y + 1));
}

bool do_edges_cross(
    size_t i,
    size_t j,
    size_t k,
    size_t l,
    const Int_ToInt_HashMap& node_to_coordinate_x,
    const Int_ToInt_HashMap& node_to_coordinate_y
) {
    size_t i_pos_x = node_to_coordinate_x.get(i);
    size_t i_pos_y = node_to_coordinate_y.get(i);
    size_t j_pos_x = node_to_coordinate_x.get(j);
    size_t j_pos_y = node_to_coordinate_y.get(j);
    size_t k_pos_x = node_to_coordinate_x.get(k);
    size_t k_pos_y = node_to_coordinate_y.get(k);
    size_t l_pos_x = node_to_coordinate_x.get(l);
    size_t l_pos_y = node_to_coordinate_y.get(l);

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

bool do_edges_cross(const GraphAttributes& attributes, size_t i, size_t j, size_t k, size_t l) {
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

size_t compute_total_crossings(const OrthogonalDrawing& result) {
    const Graph& graph = result.augmented_graph;
    const GraphAttributes& attributes = result.attributes;
    const auto [node_to_coordinate_x, node_to_coordinate_y] =
        compute_node_to_index_position(graph, attributes);
    size_t total_crossings = 0;
    vector<pair<size_t, size_t>> edges;
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (neighbor_id < node_id)
                return;
            edges.emplace_back(node_id, neighbor_id);
        });
    });
    for (size_t i = 0; i < edges.size(); ++i) {
        size_t node_1_id = edges[i].first;
        size_t node_2_id = edges[i].second;
        for (size_t j = i + 1; j < edges.size(); ++j) {
            size_t node_3_id = edges[j].first;
            size_t node_4_id = edges[j].second;
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
    print("{}", orthogonal_stats_to_string(stats));
}