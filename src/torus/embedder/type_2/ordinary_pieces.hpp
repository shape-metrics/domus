#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "nodes_positions.hpp"

namespace domus::torus {

class Bridge;
class Face;

struct CachedOrdinaryEmbedding {
  private:
    const Bridge* m_piece = nullptr;
    const NodesPositions* m_nodes_positions = nullptr;
    graph::Graph ordinary_plus_face;
    graph::Embedding embedding;
    size_t embedded_face_index;

    graph::utilities::NodesLabels<size_t> node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> edge_old_to_new_id;

    CachedOrdinaryEmbedding(const Bridge& bridge, const NodesPositions& nodes_positions);
    bool is_inside_face(const Face& face, const Bridge& bridge) const;
    void add_edge(const size_t prev_node_id, const size_t next_node_id, const size_t edge_id);
    std::pair<size_t, size_t> add_non_repeated_path(const graph::Path& non_repeated_path);
    void add_nodes_and_edges(const Face& face);
    void add_nodes_and_edges(const Bridge& bridge);

  public:
    static std::optional<CachedOrdinaryEmbedding> cache_ordinary_embedding(
        size_t face_index,
        const Face& face,
        const Bridge& bridge,
        const NodesPositions& nodes_positions
    );
    void insert_into_embedding(
        size_t assigned_face_index, const Face& assigned_face, graph::Embedding& destination
    );
    std::string to_string() const;
    void print() const;
};

} // namespace domus::torus