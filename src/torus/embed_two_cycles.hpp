#pragma once

#include <cstddef>

namespace domus::graph {
class Cycle;
class Embedding;
class Graph;
} // namespace domus::graph

namespace domus::torus {
graph::Embedding compute_embedding_of_two_cycles(
    const graph::Graph& graph,
    const graph::Cycle& cycle_1,
    graph::Cycle& cycle_2,
    const size_t intersection_node_id
);
}