#pragma once

#include <optional>

#include "domus/planarity/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::planarity {
std::optional<Embedding> embed_graph(const Graph& graph);
}