#pragma once

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Face;

void handle_type_3(graph::Graph& graph, const graph::Embedding& embedding, const Face& face);
} // namespace domus::torus
