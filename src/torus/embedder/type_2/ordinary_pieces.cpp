#include "ordinary_pieces.hpp"

#include <algorithm>
#include <iostream>

#include "domus/core/debug.hpp"
#include "domus/planarity/auslander_parter.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"

namespace domus::torus {
using namespace domus::graph;

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
    size_t face_index,
    const Face& face,
    const Bridge& bridge,
    const graph::utilities::NodesLabels<size_t>& face_node_to_pos
) {
    if (face.type() != FaceType::TYPE_1) {
        // For non-simple faces (e.g. Type 2 cylinder faces with self-intersecting or repeated paths),
        // we use the full face boundary subgraph to preserve the pinch points and multi-cycle topology.
        CachedOrdinaryEmbedding cached;
        cached.embedded_face_index = face_index;
        for (size_t i = 0; i < face.path().number_of_edges(); i++) {
            const size_t prev_node_id = face.path().get_node_id_at_position(i);
            const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
            const size_t edge_id = face.path().get_edge_id_at_position(i);

            if (!cached.node_old_to_new_id.has_label(prev_node_id)) {
                const size_t new_node = cached.ordinary_plus_face.add_node();
                cached.node_new_to_old_id.add_label(new_node, prev_node_id);
                cached.node_old_to_new_id.add_label(prev_node_id, new_node);
            }
            if (!cached.node_old_to_new_id.has_label(next_node_id)) {
                const size_t new_node = cached.ordinary_plus_face.add_node();
                cached.node_new_to_old_id.add_label(new_node, next_node_id);
                cached.node_old_to_new_id.add_label(next_node_id, new_node);
            }
            if (!cached.edge_old_to_new_id.has_label(edge_id)) {
                const size_t new_prev_node = cached.node_old_to_new_id.get_label(prev_node_id);
                const size_t new_next_node = cached.node_old_to_new_id.get_label(next_node_id);
                const size_t new_edge_id = cached.ordinary_plus_face.add_edge(new_prev_node, new_next_node);
                cached.edge_new_to_old_id.add_label(new_edge_id, edge_id);
                cached.edge_old_to_new_id.add_label(edge_id, new_edge_id);
            }
        }
        cached.add_nodes_and_edges(bridge);
        auto result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
        if (!result.has_value())
            return std::nullopt;

        cached.embedding = std::move(result.value());
        bool is_inside = false;
        const size_t attachment_id = bridge.get_attachments()[0];
        const size_t old_attachment_id = bridge.get_new_id_to_old_id().get_label(attachment_id);
        for (size_t i = 0; i < face.path().number_of_edges(); i++) {
            if (old_attachment_id == face.path().get_node_id_at_position(i)) {
                const size_t old_next = face.path().get_node_id_at_position((i + 1) % face.path().number_of_edges());
                const size_t old_prev = face.path().get_node_id_at_position((i + face.path().number_of_edges() - 1) % face.path().number_of_edges());
                const size_t next_id = cached.node_old_to_new_id.get_label(old_next);
                const size_t prev_id = cached.node_old_to_new_id.get_label(old_prev);
                const size_t att_id = cached.node_old_to_new_id.get_label(old_attachment_id);
                const size_t edge_id = cached.edge_old_to_new_id.get_label(face.path().get_edge_id_at_position(i));
                is_inside = (cached.embedding.next_in_adjacency_list(att_id, next_id, edge_id).neighbor_id == prev_id);
                break;
            }
        }
        if (!is_inside)
            cached.embedding.reverse_all_circular_orders();
        return cached;
    }

    // For simple faces (Type 1), contract the face cycle to the attachment vertices (O(K + |bridge|)).
    CachedOrdinaryEmbedding cached;
    cached.embedded_face_index = face_index;
    cached.add_nodes_and_edges(bridge);

    struct AttachmentPos {
        size_t old_id;
        size_t pos;
    };
    std::vector<AttachmentPos> attachments;
    attachments.reserve(bridge.number_of_attachments());

    for (const size_t att_id : bridge.get_attachments()) {
        const size_t old_id = bridge.get_new_id_to_old_id().get_label(att_id);
        DOMUS_ASSERT(
            face_node_to_pos.has_label(old_id),
            "CachedOrdinaryEmbedding: attachment node not found in face path"
        );
        attachments.push_back({old_id, face_node_to_pos.get_label(old_id)});
    }

    bool is_inside = false;
    auto result = std::optional<Embedding>{};
    if (attachments.empty()) {
        result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
    } else if (attachments.size() == 1) {
        const size_t a0 = cached.node_old_to_new_id.get_label(attachments[0].old_id);
        const size_t d0 = cached.ordinary_plus_face.add_node();
        const size_t d1 = cached.ordinary_plus_face.add_node();
        const size_t e1 = cached.ordinary_plus_face.add_edge(a0, d0);
        cached.ordinary_plus_face.add_edge(d0, d1);
        cached.ordinary_plus_face.add_edge(d1, a0);
        result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
        if (result.has_value()) {
            cached.embedding = std::move(result.value());
            is_inside = (cached.embedding.next_in_adjacency_list(a0, d0, e1).neighbor_id == d1);
        }
    } else {
        std::ranges::sort(attachments, {}, &AttachmentPos::pos);
        const size_t K = attachments.size();
        std::vector<size_t> new_att_ids(K);
        std::vector<size_t> dummy_ids(K);
        for (size_t i = 0; i < K; ++i) {
            new_att_ids[i] = cached.node_old_to_new_id.get_label(attachments[i].old_id);
            dummy_ids[i] = cached.ordinary_plus_face.add_node();
        }

        std::vector<size_t> forward_edges(K);
        for (size_t i = 0; i < K; ++i) {
            const size_t next_i = (i + 1) % K;
            forward_edges[i] = cached.ordinary_plus_face.add_edge(new_att_ids[i], dummy_ids[i]);
            cached.ordinary_plus_face.add_edge(dummy_ids[i], new_att_ids[next_i]);
        }

        result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
        if (result.has_value()) {
            cached.embedding = std::move(result.value());
            const size_t target_att_old_id = bridge.get_new_id_to_old_id().get_label(bridge.get_attachments()[0]);
            size_t idx = 0;
            for (size_t i = 0; i < K; ++i) {
                if (attachments[i].old_id == target_att_old_id) {
                    idx = i;
                    break;
                }
            }
            const size_t a0 = new_att_ids[idx];
            const size_t forward_neighbor = dummy_ids[idx];
            const size_t forward_edge = forward_edges[idx];
            const size_t backward_neighbor = dummy_ids[(idx + K - 1) % K];
            is_inside =
                (cached.embedding.next_in_adjacency_list(a0, forward_neighbor, forward_edge).neighbor_id ==
                 backward_neighbor);
        }
    }

    if (!result.has_value())
        return std::nullopt;

    if (!is_inside)
        cached.embedding.reverse_all_circular_orders();

    return cached;
}

std::optional<CachedOrdinaryEmbedding> CachedOrdinaryEmbedding::cache_ordinary_embedding(
    size_t face_index, const Face& face, const Bridge& bridge
) {
    graph::utilities::NodesLabels<size_t> face_node_to_pos;
    const auto& path = face.path();
    for (size_t i = 0; i < path.number_of_edges(); ++i) {
        const size_t node_id = path.get_node_id_at_position(i);
        if (!face_node_to_pos.has_label(node_id))
            face_node_to_pos.add_label(node_id, i);
    }
    return cache_ordinary_embedding(face_index, face, bridge, face_node_to_pos);
}

} // namespace domus::torus