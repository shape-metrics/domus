#pragma once

namespace domus::graph {
class GraphAttributes;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal {
using Graph = domus::graph::Graph;
using GraphAttributes = domus::graph::GraphAttributes;

void compact_area(const Graph& graph, GraphAttributes& attributes);

} // namespace domus::orthogonal