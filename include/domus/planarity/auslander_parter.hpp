#pragma once

#include <optional>

#include "domus/planarity/embedding.hpp"

class Graph;

std::optional<Embedding> embed_graph(const Graph& graph);