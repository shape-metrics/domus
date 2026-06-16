#pragma once

#include <vector>

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Face;

bool handle_type_2(
    graph::Graph& graph, graph::Embedding& embedding, const std::vector<Face>& faces
);

} // namespace domus::torus
