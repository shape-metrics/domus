#pragma once

namespace domus::graph {
class GraphAttributes;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal {

void compact_area(const graph::Graph& graph, graph::GraphAttributes& attributes);

} // namespace domus::orthogonal