#pragma once

#include <optional>

#include "domus/planarity/embedding.hpp"

class UndirectedGraph;

std::optional<Embedding> embed_graph(const UndirectedGraph& graph);