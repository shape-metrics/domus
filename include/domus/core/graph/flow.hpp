#pragma once

#include <cstddef>
#include <vector>

#include "domus/core/graph/path.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::graph::flow {

std::vector<Path> max_vertex_disjoint_paths(const Graph& graph, size_t sink_id, size_t source_id);

std::vector<Path> max_vertex_disjoint_cycles(const Graph& graph, size_t node_id);

} // namespace domus::graph::flow
