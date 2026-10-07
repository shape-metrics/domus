#pragma once

#include <optional>
#include <vector>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::ogdf_utils {

std::vector<size_t> find_kuratowski_subdivision(const graph::Graph& graph);

std::optional<graph::Embedding> compute_planar_embedding(const graph::Graph& graph);

bool is_graph_planar(const graph::Graph& graph);

} // namespace domus::ogdf_utils