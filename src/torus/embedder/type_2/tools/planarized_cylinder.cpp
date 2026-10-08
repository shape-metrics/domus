#include "planarized_cylinder.hpp"

#include <format>

#include "domus/core/debug.hpp"
#include "domus/torus/faces.hpp"

#include "adjacencies.hpp"
#include "fixed_rotation_planarity.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

std::optional<PlanarizedCylinder> PlanarizedCylinder::build(
    size_t face_index,
    const std::vector<Face>& faces,
    const graph::Embedding& partial,
    const std::vector<const Bridge*>& across_ordinary_pieces,
    const std::vector<const Bridge*>& special_pieces
) {
    PlanarizedCylinder cylinder;
    cylinder.m_face_index = face_index;
    const Face& face = faces[face_index];
    for (size_t i = 0; i < face.path().number_of_edges(); i++) {
        const size_t prev_node_id = face.path().get_node_id_at_position(i);
        const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
        const size_t edge_id = face.path().get_edge_id_at_position(i);

        if (!cylinder.m_node_old_to_new_id.has_label(prev_node_id)) {
            const size_t new_node = cylinder.m_graph.add_node();
            cylinder.m_node_new_to_old_id.add_label(new_node, prev_node_id);
            cylinder.m_node_old_to_new_id.add_label(prev_node_id, new_node);
        }
        if (!cylinder.m_node_old_to_new_id.has_label(next_node_id)) {
            const size_t new_node = cylinder.m_graph.add_node();
            cylinder.m_node_new_to_old_id.add_label(new_node, next_node_id);
            cylinder.m_node_old_to_new_id.add_label(next_node_id, new_node);
        }
        if (!cylinder.m_edge_old_to_new_id.has_label(edge_id)) {
            const size_t new_prev_node = cylinder.m_node_old_to_new_id.get_label(prev_node_id);
            const size_t new_next_node = cylinder.m_node_old_to_new_id.get_label(next_node_id);
            const size_t new_edge_id = cylinder.m_graph.add_edge(new_prev_node, new_next_node);
            cylinder.m_edge_new_to_old_id.add_label(new_edge_id, edge_id);
            cylinder.m_edge_old_to_new_id.add_label(edge_id, new_edge_id);
        }
    }

    // adding incident special pieces to planarized cylinder
    for (auto piece : special_pieces)
        cylinder.add_piece(*piece);

    // adding incident across ordinary pieces to planarized cylinder
    for (auto piece : across_ordinary_pieces) {
        cylinder.add_piece(*piece);
    }

    size_t fixed_0 = face.repeated_paths()[0].get_first_node_id();
    std::array<size_t, 3> fixed_rotation_0;
    size_t i = 0;
    for (const size_t n_id : partial.get_neighbors(fixed_0))
        fixed_rotation_0[i++] = cylinder.m_node_old_to_new_id.get_label(n_id);
    DOMUS_ASSERT(i == 3, "PlanarizedCylinder::build: fixed node does not have 3 neighbors");

    size_t fixed_1 = face.repeated_paths()[0].get_last_node_id();
    std::array<size_t, 3> fixed_rotation_1;
    i = 0;
    for (const size_t n_id : partial.get_neighbors(fixed_1))
        fixed_rotation_1[i++] = cylinder.m_node_old_to_new_id.get_label(n_id);
    DOMUS_ASSERT(i == 3, "PlanarizedCylinder::build: fixed node does not have 3 neighbors");

    auto result = frp::planar_with_fixed_rotations(
        cylinder.get_graph(),
        cylinder.m_node_old_to_new_id.get_label(fixed_0),
        fixed_rotation_0,
        cylinder.m_node_old_to_new_id.get_label(fixed_1),
        fixed_rotation_1
    );

    if (!result.has_value())
        return std::nullopt;

    cylinder.m_embedding = std::move(result.value());

    return cylinder;
}

void PlanarizedCylinder::add_piece(const Bridge& bridge) {
    for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
        const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
        if (!m_node_old_to_new_id.has_label(old_node_id)) {
            const size_t new_node_id = m_graph.add_node();
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
            const size_t new_edge_id = m_graph.add_edge(new_prev_id, new_next_id);
            m_edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
            m_edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
        }
    }
}

const Graph& PlanarizedCylinder::get_graph() const { return m_graph; }

const NodesLabels<size_t>& PlanarizedCylinder::get_node_new_to_old_id() const {
    return m_node_new_to_old_id;
}

const NodesLabels<size_t>& PlanarizedCylinder::get_node_old_to_new_id() const {
    return m_node_old_to_new_id;
}

const EdgesLabels<size_t>& PlanarizedCylinder::get_edge_new_to_old_id() const {
    return m_edge_new_to_old_id;
}

const EdgesLabels<size_t>& PlanarizedCylinder::get_edge_old_to_new_id() const {
    return m_edge_old_to_new_id;
}

void PlanarizedCylinder::merge_into_embedding(
    const Adjacencies& adjacencies, Embedding& destination_embedding
) const {
    for (size_t special_piece_index : adjacencies.special_pieces_in_face(m_face_index)) {
        const Bridge& bridge = adjacencies.get_pieces()[special_piece_index];

        // embed internal nodes and their incident edges into m_embedding
        for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
            if (bridge.is_attachment(bridge_node_id))
                continue;
            const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
            const size_t new_node_id = get_node_old_to_new_id().get_label(old_node_id);

            for (const auto edge : m_embedding.get_edges(new_node_id)) {
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
                const EdgeIter prev_edge =
                    m_embedding.prev_in_adjacency_list(new_from_id, new_to_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_from_id, old_to_id, old_edge_id, old_prev_edge_id);
            }
            if (bridge.is_attachment(bridge_edge.edge.to_id)) {
                const size_t new_to_id = get_node_old_to_new_id().get_label(old_to_id);
                const size_t new_from_id = get_node_old_to_new_id().get_label(old_from_id);
                const EdgeIter prev_edge =
                    m_embedding.prev_in_adjacency_list(new_to_id, new_from_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_to_id, old_from_id, old_edge_id, old_prev_edge_id);
            }
        }
    }
}

std::string PlanarizedCylinder::to_string() const {
    std::string result = "PlanarizedCylinder:\n";
    for (const size_t node_id : m_graph.get_nodes_ids()) {
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

} // namespace domus::torus