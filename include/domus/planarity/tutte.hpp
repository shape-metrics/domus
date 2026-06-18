#pragma once

namespace domus::graph {
class Graph;
class Attributes;
} // namespace domus::graph

namespace domus::planarity {

// assumes input graph is triconnected
void compute_nodes_positions(const graph::Graph& graph, graph::Attributes& attributes);

} // namespace domus::planarity
