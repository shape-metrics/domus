#pragma once

#include <optional>

#include "domus/core/graph/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::torus {

/**
 * @brief Computes a toroidal embedding if the input graph admits one.
 *
 * @param graph The input graph. Assumed to be biconnected.
 * @return std::optional<graph::Embedding> The output embedding.
 */
std::optional<graph::Embedding> compute_toroidal_embedding(const graph::Graph& graph);

} // namespace domus::torus