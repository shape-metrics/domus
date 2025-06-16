#include "orthogonal/drawing_stats.hpp"

#include <cmath>
#include <functional>
#include <tuple>
#include <unordered_set>
#include <vector>

std::vector<int> compute_edge_lengths(const Graph& graph,
                                      const GraphAttributes& attributes) {
  auto [node_to_coordinate_x, node_to_coordinate_y] =
      compute_node_to_index_position(graph, attributes);
  std::vector<int> edge_lengths;
  std::unordered_set<int> visited;
  for (const auto& node : graph.get_nodes()) {
    if (attributes.get_node_color(node.get_id()) != Color::BLACK) continue;
    std::function<void(int, int, int)> dfs = [&](int current_id, int black_id,
                                                 int current_length) {
      visited.insert(current_id);
      for (const auto& edge : graph.get_node_by_id(current_id).get_edges()) {
        int neighbor = edge.get_to().get_id();
        if (visited.contains(neighbor)) continue;
        int x1 = node_to_coordinate_x[current_id];
        int y1 = node_to_coordinate_y[current_id];
        int x2 = node_to_coordinate_x[neighbor];
        int y2 = node_to_coordinate_y[neighbor];
        int length = std::abs(x1 - x2) + std::abs(y1 - y2);
        Color neighbor_color = attributes.get_node_color(neighbor);
        if (neighbor_color != Color::BLACK)
          dfs(neighbor, black_id, current_length + length);
        else {
          if (black_id < neighbor) {
            int total_length = current_length + length;
            edge_lengths.push_back(total_length);
          }
        }
      }
      visited.erase(current_id);
    };
    dfs(node.get_id(), node.get_id(), 0);
  }
  return edge_lengths;
}

int compute_total_edge_length(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  std::vector<int> edge_lengths =
      compute_edge_lengths(graph, result.attributes);
  int total_edge_length = 0;
  for (const auto& length : edge_lengths) total_edge_length += length;
  return total_edge_length;
}

int compute_max_edge_length(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const auto& attributes = result.attributes;
  std::vector<int> edge_lengths = compute_edge_lengths(graph, attributes);
  int max_edge_length = 0;
  for (const auto& length : edge_lengths)
    if (length > max_edge_length) max_edge_length = length;
  return max_edge_length;
}

double compute_edge_length_std_dev(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const auto& attributes = result.attributes;
  std::vector<int> edge_lengths = compute_edge_lengths(graph, attributes);
  return compute_stddev(edge_lengths);
}

std::vector<int> compute_bends_counts(const Graph& graph,
                                      const GraphAttributes& attributes) {
  auto [node_to_coordinate_x, node_to_coordinate_y] =
      compute_node_to_index_position(graph, attributes);
  std::vector<int> bends_counts;
  for (const auto& node : graph.get_nodes()) {
    if (attributes.get_node_color(node.get_id()) != Color::BLACK) continue;
    std::unordered_set<int> visited;
    std::function<void(int, int, int, int)> dfs = [&](int current, int black,
                                                      int count,
                                                      int previous_id) {
      visited.insert(current);
      for (const auto& edge : graph.get_node_by_id(current).get_edges()) {
        int neighbor = edge.get_to().get_id();
        if (visited.contains(neighbor)) continue;
        Color neighbor_color = attributes.get_node_color(neighbor);
        if (neighbor_color != Color::BLACK) {
          if (node_to_coordinate_x[previous_id] ==
                  node_to_coordinate_x[neighbor] &&
              node_to_coordinate_y[previous_id] ==
                  node_to_coordinate_y[neighbor])
            dfs(neighbor, black, count, current);
          else
            dfs(neighbor, black, count + 1, current);
        } else if (black < neighbor) {
          if (node_to_coordinate_x[current] == node_to_coordinate_x[neighbor] &&
              node_to_coordinate_y[current] == node_to_coordinate_y[neighbor])
            count--;
          bends_counts.push_back(count);
        }
      }
      visited.erase(current);
    };
    dfs(node.get_id(), node.get_id(), 0, node.get_id());
  }
  return bends_counts;
}

int compute_total_bends(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const auto& attributes = result.attributes;
  std::vector<int> bends_counts = compute_bends_counts(graph, attributes);
  int total_bends = 0;
  for (const auto& count : bends_counts) total_bends += count;
  return total_bends;
}

int compute_max_bends_per_edge(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const auto& attributes = result.attributes;
  std::vector<int> bends_counts = compute_bends_counts(graph, attributes);
  int max_bends = 0;
  for (const auto& count : bends_counts)
    if (count > max_bends) max_bends = count;
  return max_bends;
}

double compute_bends_std_dev(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const auto& attributes = result.attributes;
  return compute_stddev(compute_bends_counts(graph, attributes));
}

int compute_total_area(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  auto [node_to_coordinate_x, node_to_coordinate_y] =
      compute_node_to_index_position(graph, result.attributes);
  int max_x = -INT_MAX;
  int max_y = -INT_MAX;
  int min_x = INT_MAX;
  int min_y = INT_MAX;
  for (const auto& node : graph.get_nodes()) {
    int node_id = node.get_id();
    int x = node_to_coordinate_x[node_id];
    int y = node_to_coordinate_y[node_id];
    max_x = std::max(max_x, x);
    max_y = std::max(max_y, y);
    min_x = std::min(min_x, x);
    min_y = std::min(min_y, y);
  }
  return (max_x - min_x + 1) * (max_y - min_y + 1);
}

bool do_edges_cross(int i, int j, int k, int l,
                    std::unordered_map<int, int>& node_to_coordinate_x,
                    std::unordered_map<int, int>& node_to_coordinate_y) {
  int i_pos_x = node_to_coordinate_x[i];
  int i_pos_y = node_to_coordinate_y[i];
  int j_pos_x = node_to_coordinate_x[j];
  int j_pos_y = node_to_coordinate_y[j];
  int k_pos_x = node_to_coordinate_x[k];
  int k_pos_y = node_to_coordinate_y[k];
  int l_pos_x = node_to_coordinate_x[l];
  int l_pos_y = node_to_coordinate_y[l];

  bool is_i_j_horizontal = i_pos_y == j_pos_y;
  bool is_k_l_horizontal = k_pos_y == l_pos_y;

  if (is_i_j_horizontal && is_k_l_horizontal) return false;
  if (!is_i_j_horizontal && !is_k_l_horizontal) return false;
  if (!is_i_j_horizontal)
    return do_edges_cross(k, l, i, j, node_to_coordinate_x,
                          node_to_coordinate_x);
  if (i_pos_x == k_pos_x || i_pos_x == l_pos_x || j_pos_x == k_pos_x ||
      j_pos_x == l_pos_x || i_pos_y == k_pos_y || i_pos_y == l_pos_y ||
      j_pos_y == k_pos_y || j_pos_y == l_pos_y)
    return false;
  if (k_pos_x < std::min(i_pos_x, j_pos_x) ||
      k_pos_x > std::max(i_pos_x, j_pos_x))
    return false;
  if (i_pos_y < std::min(k_pos_y, l_pos_y) ||
      i_pos_y > std::max(k_pos_y, l_pos_y))
    return false;
  return true;
}

bool do_edges_cross(const GraphAttributes& attributes, int i, int j, int k,
                    int l) {
  float i_pos_x = attributes.get_position_x(i);
  float i_pos_y = attributes.get_position_y(i);
  float j_pos_x = attributes.get_position_x(j);
  float j_pos_y = attributes.get_position_y(j);
  float k_pos_x = attributes.get_position_x(k);
  float k_pos_y = attributes.get_position_y(k);
  float l_pos_x = attributes.get_position_x(l);
  float l_pos_y = attributes.get_position_y(l);

  if (std::abs(i_pos_x - k_pos_x) < 0.2 || std::abs(i_pos_x - l_pos_x) < 0.2 ||
      std::abs(i_pos_y - k_pos_y) < 0.2 || std::abs(i_pos_y - l_pos_y) < 0.2 ||
      std::abs(j_pos_x - k_pos_x) < 0.2 || std::abs(j_pos_x - l_pos_x) < 0.2 ||
      std::abs(j_pos_y - k_pos_y) < 0.2 || std::abs(j_pos_y - l_pos_y) < 0.2)
    return false;

  bool is_i_j_horizontal = i_pos_y == j_pos_y;
  bool is_k_l_horizontal = k_pos_y == l_pos_y;

  if (is_i_j_horizontal && is_k_l_horizontal) {
    return (i_pos_y == k_pos_y) &&
           ((i_pos_x <= k_pos_x && j_pos_x >= k_pos_x) ||
            (i_pos_x <= l_pos_x && j_pos_x >= l_pos_x) ||
            (j_pos_x <= k_pos_x && i_pos_x >= k_pos_x) ||
            (j_pos_x <= l_pos_x && i_pos_x >= l_pos_x));
  }
  if (!is_i_j_horizontal && !is_k_l_horizontal) {
    return (i_pos_x == k_pos_x) &&
           ((i_pos_y <= k_pos_y && j_pos_y >= k_pos_y) ||
            (i_pos_y <= l_pos_y && j_pos_y >= l_pos_y) ||
            (j_pos_y <= k_pos_y && i_pos_y >= k_pos_y) ||
            (j_pos_y <= l_pos_y && i_pos_y >= l_pos_y));
  }
  if (!is_i_j_horizontal) return do_edges_cross(attributes, k, l, i, j);
  if (k_pos_x < std::min(i_pos_x, j_pos_x) ||
      k_pos_x > std::max(i_pos_x, j_pos_x))
    return false;
  if (i_pos_y < std::min(k_pos_y, l_pos_y) ||
      i_pos_y > std::max(k_pos_y, l_pos_y))
    return false;
  return true;
}

int compute_total_crossings(const DrawingResult& result) {
  const auto& graph = *result.augmented_graph;
  const GraphAttributes& attributes = result.attributes;
  auto [node_to_coordinate_x, node_to_coordinate_y] =
      compute_node_to_index_position(graph, attributes);
  int total_crossings = 0;
  for (auto& edge : graph.get_edges()) {
    int edge_id = edge.get_id();
    int i = edge.get_from().get_id();
    int j = edge.get_to().get_id();
    if (i > j) continue;
    for (auto& other_edge : graph.get_edges()) {
      int other_edge_id = other_edge.get_id();
      if (edge_id >= other_edge_id) continue;
      int k = other_edge.get_from().get_id();
      int l = other_edge.get_to().get_id();
      if (k > l) continue;
      if (i == k || i == l || j == k || j == l) continue;
      if (do_edges_cross(attributes, i, j, k, l)) ++total_crossings;
    }
  }
  return total_crossings;
}

OrthogonalStats compute_all_orthogonal_stats(const DrawingResult& result) {
  return {
      compute_total_crossings(result),    compute_total_bends(result),
      compute_total_area(result),         compute_total_edge_length(result),
      compute_max_edge_length(result),    compute_edge_length_std_dev(result),
      compute_max_bends_per_edge(result), compute_bends_std_dev(result)};
}
