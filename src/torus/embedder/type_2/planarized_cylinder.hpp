#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"

namespace domus::graph {
class Embedding;
}

namespace domus::torus {
class Adjacencies;

struct CircularOrder {
    size_t center_id;
    std::array<size_t, 3> neighbors_ids;
    static CircularOrder at_vertex(size_t node_id, const graph::Embedding& embedding);
};

class PlanarizedCylinder {
    size_t m_face_index;
    graph::Graph m_graph;
    graph::Embedding m_embedding;
    graph::utilities::NodesLabels<size_t> m_node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> m_node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> m_edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> m_edge_old_to_new_id;

    void add_piece(const Bridge& bridge);

  public:
    static std::optional<PlanarizedCylinder> build(
        size_t face_index,
        const std::vector<Face>& faces,
        const graph::Embedding& partial,
        const std::vector<const Bridge*>& across_ordinary_pieces,
        const std::vector<const Bridge*>& special_pieces
    );

    const graph::Graph& get_graph() const;
    const graph::Embedding& get_embedding() const;
    const graph::utilities::NodesLabels<size_t>& get_node_new_to_old_id() const;
    const graph::utilities::NodesLabels<size_t>& get_node_old_to_new_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_new_to_old_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_old_to_new_id() const;

    void merge_into_embedding(
        const Adjacencies& adjacencies, graph::Embedding& destination_embedding
    ) const;

    std::string to_string() const;
};

} // namespace domus::torus