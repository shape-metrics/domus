#pragma once

#include <vector>

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Face;

void handle_type_1(
    graph::Graph& graph, graph::Embedding& embedding, const std::vector<Face>& faces
);

} // namespace domus::torus
