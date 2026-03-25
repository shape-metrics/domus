#pragma once

#include <vector>

namespace domus::tree {
class Tree;
}

namespace domus::tree::algorithms {

std::vector<size_t> get_path_from_root(const Tree& tree, size_t node); // TODO should return Path?

size_t compute_common_ancestor(const Tree& tree, size_t node1, size_t node2);

} // namespace domus::tree::algorithms