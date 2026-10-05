#include "ordinary_pieces.hpp"

#include "domus/core/debug.hpp"
#include "domus/planarity/auslander_parter.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"

namespace domus::torus {
using namespace domus::graph;

void CachedOrdinaryEmbedding::add_nodes_and_edges(const Face& face) {
    for (size_t i = 0; i < face.path().number_of_edges(); i++) {
        const size_t prev_node_id = face.path().get_node_id_at_position(i);
        const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
        const size_t edge_id = face.path().get_edge_id_at_position(i);

        if (!node_old_to_new_id.has_label(prev_node_id)) {
            const size_t new_node = ordinary_plus_face.add_node();
            node_new_to_old_id.add_label(new_node, prev_node_id);
            node_old_to_new_id.add_label(prev_node_id, new_node);
        }
        if (!node_old_to_new_id.has_label(next_node_id)) {
            const size_t new_node = ordinary_plus_face.add_node();
            node_new_to_old_id.add_label(new_node, next_node_id);
            node_old_to_new_id.add_label(next_node_id, new_node);
        }
        if (!edge_old_to_new_id.has_label(edge_id)) {
            const size_t new_prev_node = node_old_to_new_id.get_label(prev_node_id);
            const size_t new_next_node = node_old_to_new_id.get_label(next_node_id);
            const size_t new_edge_id = ordinary_plus_face.add_edge(new_prev_node, new_next_node);
            edge_new_to_old_id.add_label(new_edge_id, edge_id);
            edge_old_to_new_id.add_label(edge_id, new_edge_id);
        }
    }
}

void CachedOrdinaryEmbedding::add_nodes_and_edges(const Bridge& bridge) {
    for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
        const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
        if (!node_old_to_new_id.has_label(old_node_id)) {
            const size_t new_node_id = ordinary_plus_face.add_node();
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
            const size_t new_edge_id = ordinary_plus_face.add_edge(new_prev_id, new_next_id);
            edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
            edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
        }
    }
}

std::optional<CachedOrdinaryEmbedding> CachedOrdinaryEmbedding::cache_ordinary_embedding(
    size_t face_index, const Face& face, const Bridge& bridge
) {
    // we now want to try to embed this piece into this face. we do this by building a graph
    // which is basically the union of the face and the piece and then check if it is planar.
    // if it is not then there is no hope of a future extension.
    CachedOrdinaryEmbedding cached;
    cached.embedded_face_index = face_index;

    cached.add_nodes_and_edges(face);
    cached.add_nodes_and_edges(bridge);

    auto result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
    if (!result.has_value())
        return std::nullopt;

    cached.embedding = std::move(result.value());

    if (!cached.is_inside_face(face, bridge))
        cached.embedding.reverse_all_circular_orders();

    return cached;
}

bool CachedOrdinaryEmbedding::is_inside_face(const Face& face, const Bridge& bridge) const {
    const size_t attachment_id = bridge.get_attachments()[0]; // any attachment is good
    const size_t old_attachment_id = bridge.get_new_id_to_old_id().get_label(attachment_id);
    for (size_t i = 0; i < face.path().number_of_edges(); i++) {
        const size_t old_node_id = face.path().get_node_id_at_position(i);
        const size_t old_edge_id = face.path().get_edge_id_at_position(i);
        if (old_attachment_id != old_node_id)
            continue;

        const size_t old_next_in_face_node_id =
            face.path().get_node_id_at_position((i + 1) % face.path().number_of_edges());
        const size_t old_prev_in_face_node_id = face.path().get_node_id_at_position(
            (i + face.path().number_of_edges() - 1) % face.path().number_of_edges()
        );

        const size_t next_embedding_id = node_old_to_new_id.get_label(old_next_in_face_node_id);
        const size_t prev_embedding_id = node_old_to_new_id.get_label(old_prev_in_face_node_id);
        const size_t attachment_embedding_id = node_old_to_new_id.get_label(old_attachment_id);
        const size_t edge_embedding_id = edge_old_to_new_id.get_label(old_edge_id);

        return embedding
                   .next_in_adjacency_list(
                       attachment_embedding_id,
                       next_embedding_id,
                       edge_embedding_id
                   )
                   .neighbor_id == prev_embedding_id;
    }
    DOMUS_ASSERT(false, "CachedOrdinaryEmbedding::is_inside_face: should not reach this portion.");
    return true;
}

} // namespace domus::torus