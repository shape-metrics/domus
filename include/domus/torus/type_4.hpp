#pragma once

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
void handle_type_4(const graph::Graph& graph, const graph::Embedding& embedding);
} // namespace domus::torus
