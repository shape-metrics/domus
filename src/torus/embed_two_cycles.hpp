#pragma once

#include <cstddef>
#include <utility>

#include "faces_types.hpp"

namespace domus::graph {
class Cycle;
class Embedding;
class Graph;
} // namespace domus::graph

namespace domus::torus {
std::pair<graph::Embedding, FaceType> compute_embedding_of_two_cycles(
    const graph::Graph& graph,
    const graph::Cycle& cycle_1,
    graph::Cycle& cycle_2,
    const size_t intersection_node_id
);
}