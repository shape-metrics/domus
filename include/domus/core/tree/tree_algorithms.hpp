#pragma once

#include <vector>

class Tree;

std::vector<size_t> get_path_from_root(const Tree& tree, size_t node);

size_t compute_common_ancestor(const Tree& tree, size_t node1, size_t node2);