#include "domus/core/tree/tree_algorithms.hpp"

#include <algorithm>
#include <vector>

#include "domus/core/tree/tree.hpp"

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