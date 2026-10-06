#pragma once

#include <optional>
#include <vector>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::torus {

class Bridge;
class Face;

struct CachedOrdinaryEmbedding {
  private:
    CachedOrdinaryEmbedding() = default;

  public:
    graph::Graph ordinary_plus_face;
    graph::Embedding embedding;
    size_t embedded_face_index;

    graph::utilities::NodesLabels<size_t> node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> edge_old_to_new_id;

    void add_nodes_and_edges(const Bridge& bridge);

    static std::optional<CachedOrdinaryEmbedding> cache_ordinary_embedding(
        size_t face_index,
        const Face& face,
        const Bridge& bridge,
        const graph::utilities::NodesLabels<size_t>& face_node_to_pos
    );

    static std::optional<CachedOrdinaryEmbedding>
    cache_ordinary_embedding(size_t face_index, const Face& face, const Bridge& bridge);
};

} // namespace domus::torus