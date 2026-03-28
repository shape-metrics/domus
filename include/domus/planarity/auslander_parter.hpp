#pragma once

#include <optional>

#include "domus/core/graph/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::planarity {
std::optional<graph::Embedding> compute_planar_embedding(const graph::Graph& graph);

bool is_graph_planar(const graph::Graph& graph);
} // namespace domus::planarity