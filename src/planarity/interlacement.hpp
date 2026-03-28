#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"

namespace domus::graph {
class Cycle;
}

namespace domus::planarity {
class Segment;

graph::Graph
compute_interlacement_graph(const std::vector<Segment>& segments, const graph::Cycle& cycle);
} // namespace domus::planarity