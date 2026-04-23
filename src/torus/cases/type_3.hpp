

#pragma once

#include <cstddef>

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Face;

void handle_type_3(
    graph::Graph& graph, graph::Embedding& embedding, const Face& face, size_t jolly_id
);

} // namespace domus::torus
