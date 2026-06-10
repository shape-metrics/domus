#pragma once

#include <vector>

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Bridge;
class Face;

namespace stars {

/**
 * @brief We are in the case of a Type 4 face. We guess the case in which we cannot
          embed a chord that splits the face into at most Type 2s, which means we
          need a (true/false) 3-star (or we cannot extend the face into a toroidal
          embedding).
 *
 * @param face The Type 4 face.
 * @param bridges The bridges of the face.
 * @param graph The whole graph G.
 * @param embedding The embedding of the two cycles we're trying to extend.
 * @return true Then @param embedding is extended a toroidal embedding of whole graph.
 * @return false Then @param embedding cannot be toroidally extended with a 3-star
           inside @param face and therefore @param embedding is not modified.
 */
bool try_3_stars(
    const Face& face,
    const std::vector<Bridge>& bridges,
    graph::Graph& graph,
    graph::Embedding& embedding
);

} // namespace stars

} // namespace domus::torus
