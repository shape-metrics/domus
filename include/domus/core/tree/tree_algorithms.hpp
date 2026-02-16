#ifndef MY_TREE_ALGORITHMS_H
#define MY_TREE_ALGORITHMS_H

#include <vector>

#include "domus/core/tree/tree.hpp"

class UndirectedGraph;

Tree build_spanning_tree(const UndirectedGraph& graph);

std::vector<int> get_path_from_root(const Tree& tree, int node);

int compute_common_ancestor(const Tree& tree, int node1, int node2);

#endif