#include "domus/core/tree/tree_algorithms.hpp"

#include <algorithm>
#include <queue>
#include <stddef.h>
#include <unordered_set>

#include "domus/core/graph/graph.hpp"

using namespace std;

optional<Tree> build_spanning_tree(const UndirectedGraph& graph) {
    unordered_set<int> visited;
    queue<int> queue;
    queue.push(*graph.get_nodes_ids().begin());
    Tree tree(queue.front());
    visited.insert(queue.front());
    while (!queue.empty()) {
        int node_id = queue.front();
        queue.pop();
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (!visited.contains(neighbor_id)) {
                visited.insert(neighbor_id);
                tree.add_node(neighbor_id, node_id);
                queue.push(neighbor_id);
            }
        }
    }
    for (const int node_id : graph.get_nodes_ids())
        if (!visited.contains(node_id))
            return std::nullopt;
    return tree;
}

vector<int> get_path_from_root(const Tree& tree, int node_id) {
    vector<int> path;
    path.push_back(node_id);
    while (!tree.is_root(node_id)) { // while the node has a parent
        node_id = *tree.get_parent(node_id);
        path.push_back(node_id);
    }
    ranges::reverse(path);
    return path;
}

int compute_common_ancestor(const Tree& tree, int node1, int node2) {
    const vector<int> path1 = get_path_from_root(tree, node1);
    const vector<int> path2 = get_path_from_root(tree, node2);
    size_t i = 0;
    while (i < path1.size() && i < path2.size() && path1[i] == path2[i])
        ++i;
    return path1[i - 1];
}