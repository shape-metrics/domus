#pragma once

#include <optional>

#include "domus/planarity/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::planarity {
std::optional<Embedding> compute_planar_embedding(const graph::Graph& graph);
}