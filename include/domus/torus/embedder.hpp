#pragma once

#include <optional>

#include "domus/planarity/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::torus {
std::optional<planarity::Embedding> compute_toroidal_embedding(const graph::Graph& graph);
}