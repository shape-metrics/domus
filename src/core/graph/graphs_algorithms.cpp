#include "core/graph/graphs_algorithms.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/tree/tree.hpp"
#include "core/tree/tree_algorithms.hpp"

bool is_graph_connected(const Graph& graph) {
  if (graph.size() == 0) return true;
  std::unordered_set<int> visited;
  std::vector<const GraphNode*> stack;
  stack.push_back(&(*graph.get_nodes().begin()));
  while (!stack.empty()) {
    auto* node = stack.back();
    stack.pop_back();
    visited.insert(node->get_id());
    for (auto& edge : node->get_edges())
      if (!visited.contains(edge.get_to().get_id()))
        stack.push_back(&edge.get_to());
  }
  for (auto& node : graph.get_nodes())
    if (!visited.contains(node.get_id())) return false;
  return true;
}

bool is_graph_undirected(const Graph& graph) {
  for (const auto& node : graph.get_nodes())
    for (const auto& edge : node.get_edges())
      if (!graph.has_edge(edge.get_to().get_id(), node.get_id())) return false;
  return true;
}

std::vector<Cycle> compute_all_cycles_in_undirected_graph(const Graph& graph) {
  std::vector<Cycle> all_cycles;
  std::unordered_set<int> taboo_nodes;
  for (const auto& node : graph.get_nodes()) {
    std::vector<Cycle> cycles =
        compute_all_cycles_with_node_in_undirected_graph(graph, node,
                                                         taboo_nodes);
    for (auto& cycle : cycles) all_cycles.push_back(cycle);
    taboo_nodes.insert(node.get_id());
  }
  return all_cycles;
}

std::vector<Cycle> compute_all_cycles_with_node_in_undirected_graph(
    const Graph& graph, const GraphNode& node,
    std::unordered_set<int>& taboo_nodes) {
  std::vector<Cycle> cycles;
  std::unordered_set<int> visited;
  std::function<void(int, int, std::vector<int>&)> dfs =
      [&](int current, int start, std::vector<int>& path) {
        visited.insert(current);
        path.push_back(current);
        for (const auto& edge : graph.get_node_by_id(current).get_edges()) {
          int neighbor = edge.get_to().get_id();
          if (taboo_nodes.contains(neighbor)) continue;  // skip taboo nodes
          if (neighbor == start && path.size() > 2) {    // found a cycle
            cycles.push_back(Cycle(path));
          } else if (!visited.contains(neighbor)) {
            dfs(neighbor, start, path);
          }
        }
        path.pop_back();
        visited.erase(current);
      };
  std::vector<int> path;
  dfs(node.get_id(), node.get_id(), path);
  return cycles;
}

bool dfs_find_cycle(const GraphNode& node, const Graph& graph,
                    std::unordered_map<int, int>& state,
                    std::unordered_map<int, int>& parent, int& cycle_start,
                    int& cycle_end) {
  state[node.get_id()] = 1;  // mark as visiting (gray)
  for (const auto& edge : node.get_edges()) {
    const auto& neighbor = edge.get_to();
    if (!state.contains(neighbor.get_id())) {  // unvisited
      parent[neighbor.get_id()] = node.get_id();
      if (dfs_find_cycle(neighbor, graph, state, parent, cycle_start,
                         cycle_end))
        return true;
    } else if (state[neighbor.get_id()] == 1) {
      cycle_start = neighbor.get_id();
      cycle_end = node.get_id();
      return true;
    }
  }
  state[node.get_id()] = 2;  // mark as fully processed (black)
  return false;
}

std::optional<Cycle> find_a_cycle_directed_graph(const Graph& graph) {
  std::unordered_map<int, int> state;
  std::unordered_map<int, int> parent;
  int cycle_start = -1, cycle_end = -1;
  for (const auto& node : graph.get_nodes())
    if (!state.contains(node.get_id()))
      if (dfs_find_cycle(node, graph, state, parent, cycle_start, cycle_end))
        break;
  if (cycle_start == -1) return std::nullopt;
  std::vector<int> cycle;
  for (int v = cycle_end; v != cycle_start; v = parent[v]) cycle.push_back(v);
  cycle.push_back(cycle_start);
  std::reverse(cycle.begin(), cycle.end());
  return Cycle(cycle);
}

std::vector<Cycle> compute_cycle_basis(const Graph& graph) {
  auto spanning = build_spanning_tree(graph);
  std::vector<Cycle> cycles;
  for (auto& node : graph.get_nodes()) {
    int node_id = node.get_id();
    for (auto& edge : node.get_edges()) {
      int neighbor_id = edge.get_to().get_id();
      if (node_id > neighbor_id) continue;
      if (spanning->has_edge(node_id, neighbor_id)) continue;
      int common_ancestor =
          compute_common_ancestor(*spanning, node_id, neighbor_id);
      std::vector<int> path1 = get_path_from_root(*spanning, node_id);
      std::vector<int> path2 = get_path_from_root(*spanning, neighbor_id);
      std::reverse(path1.begin(), path1.end());
      std::reverse(path2.begin(), path2.end());
      while (path1.back() != common_ancestor) path1.pop_back();
      while (path2.back() != common_ancestor) path2.pop_back();
      std::reverse(path1.begin(), path1.end());
      path1.insert(path1.end(), path2.begin(), path2.end());
      path1.pop_back();
      cycles.push_back(Cycle(path1));
    }
  }
  return cycles;
}

std::vector<int> make_topological_ordering(const Graph& graph) {
  std::unordered_map<int, int> in_degree;
  for (const auto& node : graph.get_nodes()) {
    for (const auto& edge : node.get_edges()) {
      const auto& neighbor = edge.get_to();
      if (!in_degree.contains(neighbor.get_id()))
        in_degree[neighbor.get_id()] = 0;
      in_degree[neighbor.get_id()]++;
    }
  }
  std::queue<const GraphNode*> queue;
  std::vector<int> topological_order;
  for (const auto& node : graph.get_nodes())
    if (in_degree[node.get_id()] == 0) queue.push(&node);
  int count = 0;
  while (!queue.empty()) {
    const auto node = queue.front();
    ++count;
    queue.pop();
    topological_order.push_back(node->get_id());
    for (const auto& edge : node->get_edges()) {
      const auto& neighbor = edge.get_to();
      if (--in_degree[neighbor.get_id()] == 0) queue.push(&neighbor);
    }
  }
  if (count != graph.size()) throw std::runtime_error("Graph contains cycle");
  return topological_order;
}

int compute_embedding_genus(int number_of_nodes, int number_of_edges,
                            int number_of_faces, int connected_components) {
  // f - e + v = 2(p - g)
  // f - e + v = 2p - 2g
  // 2g = 2p - f + e - v
  // g = p - (f - e + v) / 2
  return connected_components -
         (number_of_faces - number_of_edges + number_of_nodes) / 2;
}

bool are_cycles_equivalent(const Cycle& cycle1, const Cycle& cycle2) {
  if (cycle1.size() != cycle2.size()) return false;
  int v = cycle1[0];
  int v_pos_in_cycle2 = -1;
  for (int i = 0; i < cycle2.size(); ++i) {
    if (cycle2[i] == v) {
      v_pos_in_cycle2 = i;
      break;
    }
  }
  if (v_pos_in_cycle2 == -1) return false;
  bool are_the_same = true;
  for (int i = 0; i < cycle1.size(); ++i) {
    int j = v_pos_in_cycle2 + i;
    if (cycle1[i] != cycle2[j]) {
      are_the_same = false;
      break;
    }
  }
  if (are_the_same) return true;
  // Check the reverse
  for (int i = 0; i < cycle1.size(); ++i) {
    int j = v_pos_in_cycle2 - i;
    if (cycle1[i] != cycle2[j]) return false;
  }
  return true;
}

std::vector<std::unique_ptr<Graph>> compute_connected_components(
    const Graph& graph) {
  if (!is_graph_undirected(graph))
    throw std::runtime_error("Graph is not undirected");
  std::unordered_set<int> visited;
  std::vector<std::unique_ptr<Graph>> components;
  std::function<void(const GraphNode&, Graph& component)> explore_component =
      [&](const GraphNode& node, Graph& component) {
        visited.insert(node.get_id());
        for (const auto& edge : node.get_edges()) {
          const auto& neighbor = edge.get_to();
          if (!component.has_node(neighbor.get_id()))
            component.add_node(neighbor.get_id());
          if (!component.has_edge(node.get_id(), neighbor.get_id()))
            component.add_undirected_edge(node.get_id(), neighbor.get_id());
          if (!visited.contains(neighbor.get_id())) {
            explore_component(neighbor, component);
          }
        }
      };
  for (const auto& node : graph.get_nodes())
    if (!visited.contains(node.get_id())) {
      auto new_component = std::make_unique<Graph>();
      new_component->add_node(node.get_id());
      explore_component(node, *new_component);
      components.push_back(std::move(new_component));
    }
  return std::move(components);
}

void dfs_bic_com(const GraphNode& node,
                 std::unordered_map<int, int>& old_node_id_to_new_id,
                 std::unordered_map<int, int>& prev_of_node,
                 int& next_id_to_assign,
                 std::unordered_map<int, int>& low_point,
                 std::list<int>& stack_of_nodes,
                 std::list<std::pair<int, int>>& stack_of_edges,
                 std::vector<std::unique_ptr<Graph>>& components,
                 std::unordered_set<int>& cut_vertices);

BiconnectedComponents compute_biconnected_components(const Graph& graph) {
  std::unordered_map<int, int> old_node_id_to_new_id;
  std::unordered_map<int, int> prev_of_node;
  std::unordered_map<int, int> low_point;
  std::unordered_set<int> cut_vertices;
  std::vector<std::unique_ptr<Graph>> components;
  int next_id_to_assign = 0;
  std::list<int> stack_of_nodes{};
  std::list<std::pair<int, int>> stack_of_edges{};
  for (auto& node : graph.get_nodes())
    if (!old_node_id_to_new_id.contains(node.get_id()))  // node not visited
      dfs_bic_com(node, old_node_id_to_new_id, prev_of_node, next_id_to_assign,
                  low_point, stack_of_nodes, stack_of_edges, components,
                  cut_vertices);
  if (stack_of_nodes.size() > 0 || stack_of_edges.size() > 0)
    throw std::runtime_error(
        "Biconnected components algorithm did not finish correctly");
  BiconnectedComponents result;
  result.cutvertices = std::move(cut_vertices);
  result.components = std::move(components);
  return result;
}

void build_component(Graph& component, std::list<int>& nodes,
                     std::list<std::pair<int, int>>& edges);

void dfs_bic_com(const GraphNode& node,
                 std::unordered_map<int, int>& old_node_id_to_new_id,
                 std::unordered_map<int, int>& prev_of_node,
                 int& next_id_to_assign,
                 std::unordered_map<int, int>& low_point,
                 std::list<int>& stack_of_nodes,
                 std::list<std::pair<int, int>>& stack_of_edges,
                 std::vector<std::unique_ptr<Graph>>& components,
                 std::unordered_set<int>& cut_vertices) {
  int node_id = node.get_id();
  old_node_id_to_new_id[node_id] = next_id_to_assign;
  low_point[node_id] = next_id_to_assign;
  ++next_id_to_assign;
  int children_number = 0;
  for (const GraphEdge& edge : node.get_edges()) {
    const GraphNode& neighbor = edge.get_to();
    int neighbor_id = neighbor.get_id();
    if (prev_of_node.contains(node_id) && prev_of_node[node_id] == neighbor_id)
      continue;
    if (!old_node_id_to_new_id.contains(
            neighbor_id)) {  // means node is not visited
      std::list<int> new_stack_of_nodes{};
      std::list<std::pair<int, int>> new_stack_of_edges{};
      ++children_number;
      prev_of_node[neighbor_id] = node_id;
      new_stack_of_nodes.push_back(neighbor_id);
      new_stack_of_edges.push_back(std::make_pair(node_id, neighbor_id));
      dfs_bic_com(neighbor, old_node_id_to_new_id, prev_of_node,
                  next_id_to_assign, low_point, new_stack_of_nodes,
                  new_stack_of_edges, components, cut_vertices);
      if (low_point[neighbor_id] < low_point[node_id])
        low_point[node_id] = low_point[neighbor_id];
      if (low_point[neighbor_id] >= old_node_id_to_new_id[node_id]) {
        new_stack_of_nodes.push_back(node_id);
        components.push_back(std::make_unique<Graph>());
        build_component(*components.back(), new_stack_of_nodes,
                        new_stack_of_edges);
        if (prev_of_node.contains(
                node_id))  // the root needs to be handled differently
          // (handled at end of function)
          cut_vertices.insert(node_id);
      } else {
        stack_of_nodes.splice(stack_of_nodes.end(), new_stack_of_nodes);
        stack_of_edges.splice(stack_of_edges.end(), new_stack_of_edges);
      }
    } else {  // node got already visited
      int neighbor_node_id = old_node_id_to_new_id[neighbor_id];
      if (neighbor_node_id < old_node_id_to_new_id[node_id]) {
        stack_of_edges.push_back(std::make_pair(node_id, neighbor_id));
        if (neighbor_node_id < low_point[node_id])
          low_point[node_id] = neighbor_node_id;
      }
    }
  }
  if (!prev_of_node.contains(
          node_id)) {  // handling of node with no parents (the root)
    if (children_number >= 2)
      cut_vertices.insert(node_id);
    else if (children_number == 0) {  // node is isolated
      components.push_back(std::make_unique<Graph>());
      components.back()->add_node(node_id);
    }
  }
}

void build_component(Graph& component, std::list<int>& nodes,
                     std::list<std::pair<int, int>>& edges) {
  for (int node : nodes) component.add_node(node);
  for (const auto& edge : edges)
    component.add_undirected_edge(edge.first, edge.second);
}

std::string BiconnectedComponents::to_string() const {
  std::string result = "Biconnected Components:\n";
  result += "Cut vertices: ";
  for (const auto& cv : cutvertices) result += std::to_string(cv) + " ";
  result += "\nComponents:\n";
  for (const auto& component : components) {
    result += component->to_string() + "\n";
  }
  return result;
}

void BiconnectedComponents::print() const {
  std::cout << to_string() << std::endl;
}

std::pair<std::unique_ptr<Graph>, GraphEdgeHashSet>
compute_maximal_degree_4_subgraph(const Graph& graph) {
  auto subgraph = std::make_unique<Graph>();
  GraphEdgeHashSet removed_edges;
  for (const auto& node : graph.get_nodes()) subgraph->add_node(node.get_id());
  for (const auto& node : graph.get_nodes()) {
    int node_id = node.get_id();
    for (auto& edge : node.get_edges()) {
      int neighbor_id = edge.get_to().get_id();
      if (subgraph->has_edge(node_id, neighbor_id)) continue;
      if (removed_edges.contains({node_id, neighbor_id})) continue;
      if (subgraph->get_node_by_id(node_id).get_degree() < 4 &&
          subgraph->get_node_by_id(neighbor_id).get_degree() < 4) {
        subgraph->add_undirected_edge(node_id, neighbor_id);
      } else {
        removed_edges.insert({node_id, neighbor_id});
        removed_edges.insert({neighbor_id, node_id});
      }
    }
  }
  return std::make_pair(std::move(subgraph), std::move(removed_edges));
}