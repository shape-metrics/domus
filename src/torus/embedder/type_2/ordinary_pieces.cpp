#include "ordinary_pieces.hpp"

#include <format>
#include <print>

#include "domus/core/debug.hpp"
#include "domus/ogdf_utils.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"

namespace domus::torus {
using namespace domus::graph;

CachedOrdinaryEmbedding::CachedOrdinaryEmbedding(
    const Bridge& piece, const NodesPositions& nodes_positions, const Face& embedded_face
)
    : m_piece(&piece), m_nodes_positions(&nodes_positions), m_embedded_face(&embedded_face) {}

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

// TODO contraction of face useless nodes
void CachedOrdinaryEmbedding::add_nodes_and_edges(const Face& face) {
    if (face.type() == FaceType::TYPE_1) {
        for (size_t i = 0; i < face.path().number_of_edges(); i++) {
            const size_t prev_node_id = face.path().get_node_id_at_position(i);
            const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
            const size_t edge_id = face.path().get_edge_id_at_position(i);
            add_edge(prev_node_id, next_node_id, edge_id);
        }
    } else {
        // we build an equivalent square representing the cylider
        auto [new_first_0, new_last_0] = add_non_repeated_path(face.non_repeated_paths()[0]);
        auto [new_first_1, new_last_1] = add_non_repeated_path(face.non_repeated_paths()[1]);

        // link two paths together
        m_ordinary_plus_face.add_edge(new_first_0, new_last_1); // this wont receive label
        m_ordinary_plus_face.add_edge(new_first_1, new_last_0); // this wont receive label
    }
}

void CachedOrdinaryEmbedding::add_nodes_and_edges(const Bridge& bridge) {
    for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
        const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
        if (!m_node_old_to_new_id.has_label(old_node_id)) {
            const size_t new_node_id = m_ordinary_plus_face.add_node();
            m_node_new_to_old_id.add_label(new_node_id, old_node_id);
            m_node_old_to_new_id.add_label(old_node_id, new_node_id);
        }
    }
    for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
        const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
        if (!m_edge_old_to_new_id.has_label(old_edge_id)) {
            const size_t new_prev_id = m_node_old_to_new_id.get_label(
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id)
            );
            const size_t new_next_id = m_node_old_to_new_id.get_label(
                bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id)
            );
            const size_t new_edge_id = m_ordinary_plus_face.add_edge(new_prev_id, new_next_id);
            m_edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
            m_edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
        }
    }
}

std::optional<CachedOrdinaryEmbedding> CachedOrdinaryEmbedding::cache_ordinary_embedding(
    size_t face_index, const Face& face, const Bridge& bridge, const NodesPositions& nodes_positions
) {
    // we now want to try to embed this piece into this face. we do this by building a graph
    // which is basically the union of the face and the piece and then check if it is planar.
    // if it is not then there is no hope of a future extension.
    CachedOrdinaryEmbedding cached(bridge, nodes_positions, face);
    cached.m_embedded_face_index = face_index;

    cached.add_nodes_and_edges(face);
    cached.add_nodes_and_edges(bridge);

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
    size_t old_attachment_id = m_piece->get_new_id_to_old_id().get_label(
        m_piece->get_attachments()[0]
    ); // any attachment is good
    size_t attachment_position =
        m_nodes_positions->get_position_of_node_in_face(old_attachment_id, m_embedded_face_index);

    size_t old_edge_id = m_embedded_face->path().get_edge_id_at_position(attachment_position);
    size_t old_prev_edge_id = m_embedded_face->path().get_edge_id_at_position(
        (attachment_position + m_embedded_face->path().number_of_edges() - 1) %
        m_embedded_face->path().number_of_edges()
    );

    size_t new_edge_id = m_edge_old_to_new_id.get_label(old_edge_id);
    size_t new_prev_edge_id = m_edge_old_to_new_id.get_label(old_prev_edge_id);

    size_t attachment_new_id = m_node_old_to_new_id.get_label(old_attachment_id);
    for (auto e : m_embedding.get_edges(attachment_new_id)) {
        if (e.id != new_edge_id)
            continue;
        return m_embedding.next_in_adjacency_list(attachment_new_id, e.neighbor_id, e.id).id ==
               new_prev_edge_id;
    }
    DOMUS_ASSERT(false, "CachedOrdinaryEmbedding::is_inside_face: should not reach this.");
    return true;
}

size_t get_incoming_edge_to_node_in_face(const Face& face, size_t node_id) {
    const auto& path = face.path();
    for (size_t i = 0; i < path.number_of_edges(); ++i) { // TODO can be made O(1)?
        if (path.get_node_id_at_position(i) == node_id) {
            const size_t in_pos = (i == 0) ? (path.number_of_edges() - 1) : (i - 1);
            return path.get_edge_id_at_position(in_pos);
        }
    }
    DOMUS_ASSERT(false, "get_incoming_edge_to_node_in_face: node not found in face");
    return 0;
}

void CachedOrdinaryEmbedding::insert_into_embedding(Embedding& destination) const {
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
    // TODO this loop can probably made more efficient by looping on just the attachments
    for (const auto bridge_edge : m_piece->get_bridge().get_all_edges()) {
        const size_t old_edge_id = m_piece->get_new_edge_id_to_old_id().get_label(bridge_edge.id);
        const size_t old_from_id =
            m_piece->get_new_id_to_old_id().get_label(bridge_edge.edge.from_id);
        const size_t old_to_id = m_piece->get_new_id_to_old_id().get_label(bridge_edge.edge.to_id);

        if (m_piece->is_attachment(bridge_edge.edge.from_id)) {
            const size_t e_in = get_incoming_edge_to_node_in_face(*m_embedded_face, old_from_id);
            destination.add_edge_after(old_from_id, old_to_id, old_edge_id, e_in);
        }
        if (m_piece->is_attachment(bridge_edge.edge.to_id)) {
            const size_t e_in = get_incoming_edge_to_node_in_face(*m_embedded_face, old_to_id);
            destination.add_edge_after(old_to_id, old_from_id, old_edge_id, e_in);
        }
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