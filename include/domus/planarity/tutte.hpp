#pragma once

#include "domus/core/graph/graph.hpp"
namespace domus::graph {
class Graph;
class Attributes;
class Cycle;
} // namespace domus::graph

namespace domus::planarity {
void compute_nodes_positions(
    const graph::Graph& graph, graph::Attributes& attributes, const graph::Cycle& border
);
}
