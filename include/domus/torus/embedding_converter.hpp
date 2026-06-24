#pragma once

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/torus/mapping.hpp"

namespace domus::torus::mapper {

struct EquivalentEmbedding {
    graph::Graph graph;
    graph::Embedding embedding;
    graph::Attributes attributes;
    graph::utilities::NodesLabels<size_t> node_id_to_old;
    graph::utilities::EdgesLabels<size_t> edge_id_to_old;

    TorusMapping to_torus_mapping() const;
};

/**
 * @brief Given a toroidal graph, it computes an equivalent planar graph and embedding mapped to the
 * canonical square representing the torus. Every face with >3 vertices will also be augmented with
 * a wheel, so that the graph will be triconnected.
 *
 * @param graph The graph corresponding to the embedding.
 * @param embedding The toroidal embedding.
 * @return EquivalentEmbedding The struct which holds the objects which will represent the toroidal
 * embedding represented in the equivalent square.
 */
EquivalentEmbedding
build_equivalent_embedding(const graph::Graph& graph, const graph::Embedding& embedding);

} // namespace domus::torus::mapper