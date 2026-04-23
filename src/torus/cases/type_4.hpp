#pragma once

#include <cstddef>

namespace domus::graph {
class Graph;
class Embedding;
class Path;
} // namespace domus::graph

namespace domus::torus {
class Face;
class Bridge;

void handle_type_4(
    graph::Graph& graph, graph::Embedding& embedding, const Face& face, size_t jolly_id
);

} // namespace domus::torus
