#pragma once

#include <vector>

namespace domus::graph {
class Graph;
class Embedding;
class Path;
} // namespace domus::graph

namespace domus::torus {
class Face;

void handle_type_3(
    graph::Graph& graph, graph::Embedding& embedding, const std::vector<graph::Path>& faces
);
} // namespace domus::torus
