#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"

namespace domus::ogdf_utils {

std::vector<size_t> find_kuratowski_subdivision(const graph::Graph& graph);

}