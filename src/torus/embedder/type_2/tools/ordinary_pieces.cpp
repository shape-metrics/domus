#include "ordinary_pieces.hpp"

#include <format>
#include <print>

#include "domus/core/debug.hpp"
#include "domus/ogdf_utils.hpp"
#include "domus/torus/bridge.hpp"
#include "domus/torus/faces.hpp"

#include "nodes_positions.hpp"

namespace domus::torus {
using namespace domus::graph;

CachedOrdinaryEmbedding::CachedOrdinaryEmbedding(const Bridge& piece) : m_piece(&piece) {}

void CachedOrdinaryEmbedding::add_edge(
    const size_t prev_node_id, const size_t next_node_id, const size_t edge_id
) {
    if (!m_node_old_to_new_id.has_label(prev_node_id)) {
        const size_t new_node = m_ordinary_plus_face.add_node();
        m_node_new_to_old_id.add_label(new_node, prev_node_id);
        m_node_old_to_new_id.add_label(prev_node_id, new_node);
    }
    if (!m_node_old_to_new_id.has_label(next_node_id)) {
        const size_t new_node = m_ordinary_plus_face.add_node();
        m_node_new_to_old_id.add_label(new_node, next_node_id);
        m_node_old_to_new_id.add_label(next_node_id, new_node);
    }
    if (!m_edge_old_to_new_id.has_label(edge_id)) {
        const size_t new_prev_node = m_node_old_to_new_id.get_label(prev_node_id);
        const size_t new_next_node = m_node_old_to_new_id.get_label(next_node_id);
        const size_t new_edge_id = m_ordinary_plus_face.add_edge(new_prev_node, new_next_node);
        m_edge_new_to_old_id.add_label(new_edge_id, edge_id);
        m_edge_old_to_new_id.add_label(edge_id, new_edge_id);
    }
}

// returns new endpoints of the non repeated path
std::pair<size_t, size_t>
CachedOrdinaryEmbedding::add_non_repeated_path(const Path& non_repeated_path) {
    for (size_t i = 1; i < non_repeated_path.number_of_edges() - 1; i++) {
        const size_t prev_node_id = non_repeated_path.get_node_id_at_position(i);
        const size_t next_node_id = non_repeated_path.get_node_id_at_position(i + 1);
        const size_t edge_id = non_repeated_path.get_edge_id_at_position(i);
        add_edge(prev_node_id, next_node_id, edge_id);
    }

    // originally the first and the last in the repeated path are the same node.
    // we separate them to build the square
    size_t new_first = m_ordinary_plus_face.add_node();
    size_t new_last = m_ordinary_plus_face.add_node();

    m_node_new_to_old_id.add_label(new_first, non_repeated_path.get_first_node_id());
    m_node_new_to_old_id.add_label(new_last, non_repeated_path.get_first_node_id());

    size_t second = non_repeated_path.get_node_id_at_position(1);
    size_t second_last =
        non_repeated_path.get_node_id_at_position(non_repeated_path.number_of_edges() - 1);

    size_t new_second = m_node_old_to_new_id.get_label(second);
    size_t new_second_last = m_node_old_to_new_id.get_label(second_last);

    size_t new_first_edge = m_ordinary_plus_face.add_edge(new_first, new_second);
    size_t new_last_edge = m_ordinary_plus_face.add_edge(new_second_last, new_last);

    m_edge_new_to_old_id.add_label(new_first_edge, non_repeated_path.get_first_edge_id());
    m_edge_new_to_old_id.add_label(new_last_edge, non_repeated_path.get_last_edge_id());
    m_edge_old_to_new_id.add_label(non_repeated_path.get_first_edge_id(), new_first_edge);
    m_edge_old_to_new_id.add_label(non_repeated_path.get_last_edge_id(), new_last_edge);

    return {new_first, new_last};
}

void CachedOrdinaryEmbedding::compute_contracted_face(
    const NodesPositions& nodes_positions, size_t face_index
) {
    m_contracted_face.reserve(m_piece->number_of_attachments());
    for (const size_t old_attachment : m_piece->get_old_attachments())
        m_contracted_face.emplace_back(
            old_attachment,
            nodes_positions.get_position_of_node_in_face(old_attachment, face_index)
        );

    std::sort(m_contracted_face.begin(), m_contracted_face.end(), [](auto& a, auto& b) {
        return a.second < b.second;
    });
}

void CachedOrdinaryEmbedding::add_nodes_and_edges_from_face() {
    if (m_contracted_face.size() == 2) {
        // to avoid multiple edges
        const size_t old_attachment_0 = m_contracted_face[0].first;
        const size_t old_attachment_1 = m_contracted_face[1].first;

        const size_t new_attachment_0 = m_ordinary_plus_face.add_node();
        m_node_new_to_old_id.add_label(new_attachment_0, old_attachment_0);
        m_node_old_to_new_id.add_label(old_attachment_0, new_attachment_0);

        const size_t new_attachment_1 = m_ordinary_plus_face.add_node();
        m_node_new_to_old_id.add_label(new_attachment_1, old_attachment_1);
        m_node_old_to_new_id.add_label(old_attachment_1, new_attachment_1);

        const size_t in_betweener_0_1 = m_ordinary_plus_face.add_node();
        const size_t in_betweener_1_0 = m_ordinary_plus_face.add_node();

        m_ordinary_plus_face.add_edge(new_attachment_0, in_betweener_0_1);
        m_ordinary_plus_face.add_edge(in_betweener_0_1, new_attachment_1);
        m_ordinary_plus_face.add_edge(new_attachment_1, in_betweener_1_0);
        m_ordinary_plus_face.add_edge(in_betweener_1_0, new_attachment_0);

        m_is_inside_circular_order = {new_attachment_0, in_betweener_1_0, in_betweener_0_1};
    } else {
        for (size_t i = 0; i < m_contracted_face.size(); i++) {
            const size_t prev_node_id = m_contracted_face[i].first;
            const size_t next_node_id = m_contracted_face[(i + 1) % m_contracted_face.size()].first;

            if (!m_node_old_to_new_id.has_label(prev_node_id)) {
                const size_t new_node = m_ordinary_plus_face.add_node();
                m_node_new_to_old_id.add_label(new_node, prev_node_id);
                m_node_old_to_new_id.add_label(prev_node_id, new_node);
            }
            if (!m_node_old_to_new_id.has_label(next_node_id)) {
                const size_t new_node = m_ordinary_plus_face.add_node();
                m_node_new_to_old_id.add_label(new_node, next_node_id);
                m_node_old_to_new_id.add_label(next_node_id, new_node);
            }

            const size_t new_prev_node = m_node_old_to_new_id.get_label(prev_node_id);
            const size_t new_next_node = m_node_old_to_new_id.get_label(next_node_id);
            m_ordinary_plus_face.add_edge(new_prev_node, new_next_node);
        }

        const size_t new_prev_node_id = m_node_old_to_new_id.get_label(m_contracted_face[0].first);
        const size_t new_center_node_id =
            m_node_old_to_new_id.get_label(m_contracted_face[1].first);
        const size_t new_next_node_id = m_node_old_to_new_id.get_label(m_contracted_face[2].first);
        m_is_inside_circular_order = {new_center_node_id, new_prev_node_id, new_next_node_id};
    }
}

void CachedOrdinaryEmbedding::add_nodes_and_edges_from_piece() {
    for (const size_t bridge_node_id : m_piece->get_bridge().get_nodes_ids()) {
        const size_t old_node_id = m_piece->get_new_id_to_old_id().get_label(bridge_node_id);
        if (!m_node_old_to_new_id.has_label(old_node_id)) {
            const size_t new_node_id = m_ordinary_plus_face.add_node();
            m_node_new_to_old_id.add_label(new_node_id, old_node_id);
            m_node_old_to_new_id.add_label(old_node_id, new_node_id);
        }
    }
    for (const auto bridge_edge : m_piece->get_bridge().get_all_edges()) {
        const size_t old_edge_id = m_piece->get_new_edge_id_to_old_id().get_label(bridge_edge.id);
        if (!m_edge_old_to_new_id.has_label(old_edge_id)) {
            const size_t new_prev_id = m_node_old_to_new_id.get_label(
                m_piece->get_new_id_to_old_id().get_label(bridge_edge.edge.from_id)
            );
            const size_t new_next_id = m_node_old_to_new_id.get_label(
                m_piece->get_new_id_to_old_id().get_label(bridge_edge.edge.to_id)
            );
            const size_t new_edge_id = m_ordinary_plus_face.add_edge(new_prev_id, new_next_id);
            m_edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
            m_edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
        }
    }
}

std::optional<CachedOrdinaryEmbedding> CachedOrdinaryEmbedding::cache_ordinary_embedding(
    size_t face_index, const Bridge& piece, const NodesPositions& nodes_positions
) {
    // we now want to try to embed this piece into this face. we do this by building a graph
    // which is basically the union of the face and the piece and then check if it is planar.
    // if it is not then there is no hope of a future extension.
    CachedOrdinaryEmbedding cached(piece);
    cached.m_embedded_face_index = face_index;
    cached.compute_contracted_face(nodes_positions, face_index);

    cached.add_nodes_and_edges_from_face();
    cached.add_nodes_and_edges_from_piece();

    auto result = ogdf_utils::compute_planar_embedding(cached.m_ordinary_plus_face);
    if (!result.has_value())
        return std::nullopt;

    cached.m_embedding = std::move(result.value());

    // we strincly want that in the computed embedding, if we traverse the face counter clockwise
    // (as it is actually represented its path) than the piece got embedded inside that face.
    if (!cached.is_inside_face())
        cached.m_embedding.reverse_all_circular_orders();

    return cached;
}

bool CachedOrdinaryEmbedding::is_inside_face() const {
    auto [new_center_id, new_prev_id, new_next_id] = m_is_inside_circular_order.value();
    size_t edge_id = m_ordinary_plus_face.get_edge_id(new_center_id, new_next_id);
    return m_embedding.next_in_adjacency_list(new_center_id, new_next_id, edge_id).neighbor_id ==
           new_prev_id;
}

void CachedOrdinaryEmbedding::insert_into_face(
    Embedding& destination, const Face& face, size_t face_id, const NodesPositions& nodes_positions
) const {
    const Embedding& piece_embedding = m_embedding;

    // embed internal nodes and their incident edges into m_embedding
    for (const size_t bridge_node_id : m_piece->get_bridge().get_nodes_ids()) {
        if (m_piece->is_attachment(bridge_node_id))
            continue;
        const size_t old_node_id = m_piece->get_new_id_to_old_id().get_label(bridge_node_id);
        const size_t new_node_id = m_node_old_to_new_id.get_label(old_node_id);

        for (const auto edge : piece_embedding.get_edges(new_node_id)) {
            const size_t old_neighbor_id = m_node_new_to_old_id.get_label(edge.neighbor_id);
            const size_t old_edge_id = m_edge_new_to_old_id.get_label(edge.id);
            destination.add_edge(old_node_id, old_neighbor_id, old_edge_id);
        }
    }

    // embed bridge edges incident to attachments into m_embedding
    for (size_t piece_attachment_id : m_piece->get_attachments()) {
        size_t old_attachment_id = m_piece->get_new_id_to_old_id().get_label(piece_attachment_id);
        size_t position = nodes_positions.get_position_of_node_in_face(old_attachment_id, face_id);

        size_t old_next_edge_id = face.path().get_edge_id_at_position(position);

        auto [old_leg_neighbor_id, old_leg_edge_id] = [&]() -> std::pair<size_t, size_t> {
            DOMUS_ASSERT(
                m_piece->get_bridge().get_degree_of_node(piece_attachment_id) == 1,
                "CachedOrdinaryEmbedding::insert_into_face: attachment should have only 1 leg"
            );
            for (const auto& edge : m_piece->get_bridge().get_edges(piece_attachment_id)) {
                return {
                    m_piece->get_new_id_to_old_id().get_label(edge.neighbor_id),
                    m_piece->get_new_edge_id_to_old_id().get_label(edge.id)
                };
            }
            return {0, 0};
        }();

        destination.add_edge_before(
            old_attachment_id,
            old_leg_neighbor_id,
            old_leg_edge_id,
            old_next_edge_id
        );
    }
}

std::string CachedOrdinaryEmbedding::to_string() const {
    std::string result = "Cached:\n";
    for (const size_t node_id : m_ordinary_plus_face.get_nodes_ids()) {
        const size_t old_node_id = m_node_new_to_old_id.get_label(node_id);
        result += std::format("{} [ ", old_node_id);
        for (const auto edge : m_embedding.get_edges(node_id)) {
            const size_t old_neighbor_id = m_node_new_to_old_id.get_label(edge.neighbor_id);
            result += std::format("{} ", old_neighbor_id);
        }
        result += "]\n";
    }
    return result;
}

void CachedOrdinaryEmbedding::print() const { std::println("{}", to_string()); }

} // namespace domus::torus
