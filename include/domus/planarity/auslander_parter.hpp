#ifndef MY_EMBEDDER_H
#define MY_EMBEDDER_H

#include <optional>

#include "domus/planarity/embedding.hpp"

class UndirectedGraph;

std::optional<Embedding> embed_graph(const UndirectedGraph& graph);

#endif