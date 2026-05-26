#pragma once

#include <cstddef>

namespace domus::graph {
class Graph;
class Embedding;
class Path;
} // namespace domus::graph

namespace domus::torus {
class Face;
class Bridge;

/**
 * @brief We are in the case of a Type 4 face. This function returns an extension
          of the face into a toroidal embedding of the whole graphs in case it
          admits one.
 *
 * @param graph The whole graph G.
 * @param embedding The current toroidal embedding to extend (2 cycles).
 * @param face The only face corresponding to the embedding, which has to be of Type 4.
 * @param jolly_id An id of a vertex (not in G) used to create artificial paths
                   inserted in G to split the face.
 * @return true Then @param embedding is extended a toroidal embedding of whole graph.
 * @return false Then @param embedding cannot be toroidally extended, and therefore
           it is not modified.
 */
bool handle_type_4(
    graph::Graph& graph, graph::Embedding& embedding, const Face& face, size_t jolly_id
);

} // namespace domus::torus
