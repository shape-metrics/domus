#pragma once

namespace domus::graph {
class Attributes;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal {

void compact_area(const graph::Graph& graph, graph::Attributes& attributes);

} // namespace domus::orthogonal