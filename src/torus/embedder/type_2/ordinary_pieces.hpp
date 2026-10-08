#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "nodes_positions.hpp"

namespace domus::torus {

class Bridge;
class Face;

class CachedOrdinaryEmbedding {
    const Bridge* m_piece = nullptr;
    const NodesPositions* m_nodes_positions = nullptr;
    graph::Graph m_ordinary_plus_face;
    graph::Embedding m_embedding;
    size_t m_embedded_face_index;
    const Face* m_embedded_face = nullptr;

    graph::utilities::NodesLabels<size_t> m_node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> m_node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> m_edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> m_edge_old_to_new_id;

    CachedOrdinaryEmbedding(
        const Bridge& bridge, const NodesPositions& nodes_positions, const Face& face
    );
    bool is_inside_face() const;
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
    void insert_into_embedding(graph::Embedding& destination) const;
    std::string to_string() const;
    void print() const;
};

} // namespace domus::torus