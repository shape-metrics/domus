#include "domus/core/tree/tree_algorithms.hpp"

#include <algorithm>
#include <queue>
#include <stddef.h>

#include "domus/core/graph/graph.hpp"

std::optional<Tree> build_spanning_tree(const Graph& graph) {
    NodesContainer visited;
    std::queue<size_t> queue;
    queue.push(graph.get_one_node_id());
    Tree tree(queue.front());
    visited.add_node(queue.front());
    while (!queue.empty()) {
        size_t node_id = queue.front();
        queue.pop();
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!visited.has_node(neighbor_id)) {
                visited.add_node(neighbor_id);
                tree.add_node(neighbor_id, node_id);
                queue.push(neighbor_id);
            }
        });
    }
    if (tree.size() != graph.size())
        return std::nullopt;
    return tree;
}

std::vector<size_t> get_path_from_root(const Tree& tree, size_t node_id) {
    std::vector<size_t> path;
    path.push_back(node_id);
    while (!tree.is_root(node_id)) { // while the node has a parent
        node_id = tree.get_parent(node_id);
        path.push_back(node_id);
    }
    std::ranges::reverse(path);
    return path;
}

size_t compute_common_ancestor(const Tree& tree, size_t node1, size_t node2) {
    std::vector<size_t> path1 = get_path_from_root(tree, node1);
    std::vector<size_t> path2 = get_path_from_root(tree, node2);
    size_t i = 0;
    while (i < path1.size() && i < path2.size() && path1[i] == path2[i])
        ++i;
    return path1[i - 1];
}