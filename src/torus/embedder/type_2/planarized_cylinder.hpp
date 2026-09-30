#pragma once

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "adjacencies.hpp"

namespace domus::graph {
class Embedding;
}

namespace domus::torus {

class PlanarizedCylinder {
    size_t face_index;
    graph::Graph graph;
    graph::utilities::NodesLabels<size_t> node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> edge_old_to_new_id;

  public:
    static PlanarizedCylinder build(size_t face_index, const Adjacencies& adjacencies);

    void add_piece(const Bridge& bridge);

    const graph::Graph& get_graph() const;
    const graph::utilities::NodesLabels<size_t>& get_node_new_to_old_id() const;
    const graph::utilities::NodesLabels<size_t>& get_node_old_to_new_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_new_to_old_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_old_to_new_id() const;

    void embed_special_pieces_from_cylinder_into_embedding(
        const graph::Embedding& cylinder_embedding,
        const Adjacencies& adjacencies,
        graph::Embedding& destination_embedding
    ) const;
};

} // namespace domus::torus