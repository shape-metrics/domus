#include "planarized_cylinder.hpp"

#include "domus/core/graph/embedding.hpp"

namespace domus::torus {

PlanarizedCylinder PlanarizedCylinder::build(size_t face_index, const Adjacencies& adjacencies) {
    PlanarizedCylinder cylinder;
    cylinder.face_index = face_index;
    const Face& face = adjacencies.get_faces()[face_index];
    for (size_t i = 0; i < face.path().number_of_edges(); i++) {
        const size_t prev_node_id = face.path().get_node_id_at_position(i);
        const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
        const size_t edge_id = face.path().get_edge_id_at_position(i);

        if (!cylinder.node_old_to_new_id.has_label(prev_node_id)) {
            const size_t new_node = cylinder.graph.add_node();
            cylinder.node_new_to_old_id.add_label(new_node, prev_node_id);
            cylinder.node_old_to_new_id.add_label(prev_node_id, new_node);
        }
        if (!cylinder.node_old_to_new_id.has_label(next_node_id)) {
            const size_t new_node = cylinder.graph.add_node();
            cylinder.node_new_to_old_id.add_label(new_node, next_node_id);
            cylinder.node_old_to_new_id.add_label(next_node_id, new_node);
        }
        if (!cylinder.edge_old_to_new_id.has_label(edge_id)) {
            const size_t new_prev_node = cylinder.node_old_to_new_id.get_label(prev_node_id);
            const size_t new_next_node = cylinder.node_old_to_new_id.get_label(next_node_id);
            const size_t new_edge_id = cylinder.graph.add_edge(new_prev_node, new_next_node);
            cylinder.edge_new_to_old_id.add_label(new_edge_id, edge_id);
            cylinder.edge_old_to_new_id.add_label(edge_id, new_edge_id);
        }
    }

    // adding incident special pieces to planarized cylinder
    for (size_t piece_index : adjacencies.special_pieces_in_face(face_index))
        cylinder.add_piece(adjacencies.get_pieces()[piece_index]);

    return cylinder;
}

void PlanarizedCylinder::add_piece(const Bridge& bridge) {
    for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
        const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
        if (!node_old_to_new_id.has_label(old_node_id)) {
            const size_t new_node_id = graph.add_node();
            node_new_to_old_id.add_label(new_node_id, old_node_id);
            node_old_to_new_id.add_label(old_node_id, new_node_id);
        }
    }
    for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
        const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
        if (!edge_old_to_new_id.has_label(old_edge_id)) {
            const size_t new_prev_id = node_old_to_new_id.get_label(
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id)
            );
            const size_t new_next_id = node_old_to_new_id.get_label(
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id)
            );
            const size_t new_edge_id = graph.add_edge(new_prev_id, new_next_id);
            edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
            edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
        }
    }
}

const graph::Graph& PlanarizedCylinder::get_graph() const { return graph; }

const graph::utilities::NodesLabels<size_t>& PlanarizedCylinder::get_node_new_to_old_id() const {
    return node_new_to_old_id;
}

const graph::utilities::NodesLabels<size_t>& PlanarizedCylinder::get_node_old_to_new_id() const {
    return node_old_to_new_id;
}

const graph::utilities::EdgesLabels<size_t>& PlanarizedCylinder::get_edge_new_to_old_id() const {
    return edge_new_to_old_id;
}

const graph::utilities::EdgesLabels<size_t>& PlanarizedCylinder::get_edge_old_to_new_id() const {
    return edge_old_to_new_id;
}

void PlanarizedCylinder::embed_special_pieces_from_cylinder_into_embedding(
    const graph::Embedding& cylinder_embedding,
    const Adjacencies& adjacencies,
    graph::Embedding& destination_embedding
) const {
    for (size_t special_piece_index : adjacencies.special_pieces_in_face(face_index)) {
        const Bridge& bridge = adjacencies.get_pieces()[special_piece_index];

        // embed internal nodes and their incident edges into m_embedding
        for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
            if (bridge.is_attachment(bridge_node_id))
                continue;
            const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
            const size_t new_node_id = get_node_old_to_new_id().get_label(old_node_id);

            for (const auto edge : cylinder_embedding.get_edges(new_node_id)) {
                const size_t old_neighbor_id = get_node_new_to_old_id().get_label(edge.neighbor_id);
                const size_t old_edge_id = get_edge_new_to_old_id().get_label(edge.id);
                destination_embedding.add_edge(old_node_id, old_neighbor_id, old_edge_id);
            }
        }

        // embed bridge edges incident to attachments into m_embedding
        for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
            const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
            const size_t old_from_id =
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id);
            const size_t old_to_id =
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id);

            const size_t new_edge_id = get_edge_old_to_new_id().get_label(old_edge_id);

            if (bridge.is_attachment(bridge_edge.edge.from_id)) {
                const size_t new_from_id = get_node_old_to_new_id().get_label(old_from_id);
                const size_t new_to_id = get_node_old_to_new_id().get_label(old_to_id);
                const graph::EdgeIter prev_edge =
                    cylinder_embedding.prev_in_adjacency_list(new_from_id, new_to_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_from_id, old_to_id, old_edge_id, old_prev_edge_id);
            }
            if (bridge.is_attachment(bridge_edge.edge.to_id)) {
                const size_t new_to_id = get_node_old_to_new_id().get_label(old_to_id);
                const size_t new_from_id = get_node_old_to_new_id().get_label(old_from_id);
                const graph::EdgeIter prev_edge =
                    cylinder_embedding.prev_in_adjacency_list(new_to_id, new_from_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_to_id, old_from_id, old_edge_id, old_prev_edge_id);
            }
        }
    }
}

} // namespace domus::torus