#pragma once

#include <optional>
#include <vector>

#include "domus/core/tree/tree.hpp"

class Graph;

std::optional<Tree> build_spanning_tree(const Graph& graph);

std::vector<int> get_path_from_root(const Tree& tree, int node);

int compute_common_ancestor(const Tree& tree, int node1, int node2);