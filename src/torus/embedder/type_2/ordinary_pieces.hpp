#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::torus {

class Bridge;
class Face;

struct CachedOrdinaryEmbedding {
  private:
    CachedOrdinaryEmbedding() = default;
    bool is_inside_face(const Face& face, const Bridge& bridge) const;
    void add_nodes_and_edges(const Face& face);
    void add_nodes_and_edges(const Bridge& bridge);

  public:
    graph::Graph ordinary_plus_face;
    graph::Embedding embedding;
    size_t embedded_face_index;

    graph::utilities::NodesLabels<size_t> node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> edge_old_to_new_id;

    static std::optional<CachedOrdinaryEmbedding>
    cache_ordinary_embedding(size_t face_index, const Face& face, const Bridge& bridge);
};

} // namespace domus::torus