#pragma once

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/torus/faces.hpp"

namespace domus::torus::mapper {

struct EquivalentEmbedding {
    graph::Graph graph;
    graph::Embedding embedding;
    graph::Attributes attributes;
    graph::utilities::NodesLabels<size_t> node_id_to_old;
    graph::utilities::EdgesLabels<size_t> edge_id_to_old;
};

/**
 * @brief Given a toroidal graph and a face of either Type 3 or Type 4, it computes an equivalent
 * planar graph in which the input face is represented as the outer face (in it the repeated paths
 * will be represented twice). Every face with >3 vertices will also be augmented with a wheel, so
 * that the graph will be triconnected.
 *
 * @param graph The graph corresponding to the embedding.
 * @param embedding The toroidal embedding.
 * @param outer_face The Type 3/4 face that will be the outer face.
 * @return EquivalentEmbedding The struct which holds the objects which will represent the toroidal
 * embedding represented in the equivalent square.
 */
EquivalentEmbedding build_equivalent_embedding(
    const graph::Graph& graph, const graph::Embedding& embedding, const Face& outer_face
);

} // namespace domus::torus::mapper