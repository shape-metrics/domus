#include "domus/core/graph/palm_tree.hpp"

// #include <stack>

// #include "domus/core/graph/graph.hpp"
// #include "domus/core/tree/tree.hpp"

/*PalmTree compute_palm_tree(const Graph& graph) {
  const int starting_id = *graph.get_nodes_ids().begin();
  auto tree = std::make_unique<Tree>(starting_id);
  std::unordered_map<int, int> node_to_label;
  node_to_label[starting_id] = 0;
  int next_label = 1;
  std::stack<int> stack;
  stack.push(starting_id);
  while (!stack.empty()) {
    const int node_id = stack.top();
    stack.pop();
    for (const int neighbor_id : graph.get_adjacency_list(node_id)) {
      if (node_to_label.contains(neighbor_id)) continue;
      node_to_label[neighbor_id] = next_label;
      next_label++;
      stack.push(neighbor_id);
    }
}
}
*/