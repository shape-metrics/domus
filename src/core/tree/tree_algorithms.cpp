#include "core/tree/tree_algorithms.hpp"

#include <algorithm>
#include <cassert>
#include <list>
#include <unordered_set>

std::unique_ptr<Tree> build_spanning_tree(const Graph& graph) {
  std::unordered_set<int> visited;
  std::list<const GraphNode*> queue;
  queue.push_back(&(*graph.get_nodes().begin()));
  auto tree = std::make_unique<Tree>(queue.front()->get_id());
  visited.insert(queue.front()->get_id());
  while (!queue.empty()) {
    auto* node = queue.front();
    queue.pop_front();
    for (auto& edge : node->get_edges()) {
      int neighbor_index = edge.get_to().get_id();
      if (!visited.contains(neighbor_index)) {
        visited.insert(neighbor_index);
        tree->add_node(neighbor_index, node->get_id());
        queue.push_back(&edge.get_to());
      }
    }
  }
  for (auto& node : graph.get_nodes()) assert(visited.contains(node.get_id()));
  return tree;
}

std::vector<int> get_path_from_root(const Tree& tree, int node_id) {
  std::vector<int> path;
  const TreeNode* node = &tree.get_node_by_id(node_id);
  path.push_back(node->get_id());
  while (!node->is_root()) {  // while node has a parent
    node = &node->get_parent();
    path.push_back(node->get_id());
  }
  std::reverse(path.begin(), path.end());
  return path;
}

int compute_common_ancestor(const Tree& tree, int node1, int node2) {
  std::vector<int> path1 = get_path_from_root(tree, node1);
  std::vector<int> path2 = get_path_from_root(tree, node2);
  int i = 0;
  while (i < path1.size() && i < path2.size() && path1[i] == path2[i]) ++i;
  return path1[i - 1];
}