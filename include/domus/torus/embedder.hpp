#pragma once

#include <optional>

#include "domus/core/graph/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::torus {
std::optional<graph::Embedding> compute_toroidal_embedding(const graph::Graph& graph);
}