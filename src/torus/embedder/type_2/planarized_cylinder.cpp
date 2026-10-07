#include "planarized_cylinder.hpp"

#include "domus/core/debug.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/ogdf_utils.hpp"

namespace domus::torus {
using namespace domus::graph;
using domus::graph::algorithms::BiconnectedComponents;
using namespace domus::graph::utilities;

std::optional<PlanarizedCylinder> PlanarizedCylinder::build(
    size_t face_index,
    const Adjacencies& adjacencies,
    const std::vector<size_t>& across_ordinary_pieces
) {
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

    // adding incident across ordinary pieces to planarized cylinder
    for (size_t piece_index : across_ordinary_pieces) {
        DOMUS_ASSERT(
            adjacencies.is_ordinary_piece_cutting_cylinder(piece_index, face_index),
            "PlanarizedCylinder::build: one of the across ordinary pieces is not across"
        );
        cylinder.add_piece(adjacencies.get_pieces()[piece_index]);
    }

    auto result = ogdf_utils::compute_planar_embedding(cylinder.get_graph());
    if (!result.has_value())
        return std::nullopt;

    cylinder.embedding = std::move(result.value());
    cylinder.adjust_rotation_scheme(face);

    return cylinder;
}

std::optional<PlanarizedCylinder>
PlanarizedCylinder::build(size_t face_index, const Adjacencies& adjacencies) {
    return PlanarizedCylinder::build(face_index, adjacencies, {});
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

const Graph& PlanarizedCylinder::get_graph() const { return graph; }

const NodesLabels<size_t>& PlanarizedCylinder::get_node_new_to_old_id() const {
    return node_new_to_old_id;
}

const NodesLabels<size_t>& PlanarizedCylinder::get_node_old_to_new_id() const {
    return node_old_to_new_id;
}

const EdgesLabels<size_t>& PlanarizedCylinder::get_edge_new_to_old_id() const {
    return edge_new_to_old_id;
}

const EdgesLabels<size_t>& PlanarizedCylinder::get_edge_old_to_new_id() const {
    return edge_old_to_new_id;
}

void PlanarizedCylinder::merge_into_embedding(
    const Adjacencies& adjacencies, Embedding& destination_embedding
) const {
    for (size_t special_piece_index : adjacencies.special_pieces_in_face(face_index)) {
        const Bridge& bridge = adjacencies.get_pieces()[special_piece_index];

        // embed internal nodes and their incident edges into m_embedding
        for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
            if (bridge.is_attachment(bridge_node_id))
                continue;
            const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
            const size_t new_node_id = get_node_old_to_new_id().get_label(old_node_id);

            for (const auto edge : embedding.get_edges(new_node_id)) {
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
                    embedding.prev_in_adjacency_list(new_from_id, new_to_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_from_id, old_to_id, old_edge_id, old_prev_edge_id);
            }
            if (bridge.is_attachment(bridge_edge.edge.to_id)) {
                const size_t new_to_id = get_node_old_to_new_id().get_label(old_to_id);
                const size_t new_from_id = get_node_old_to_new_id().get_label(old_from_id);
                const EdgeIter prev_edge =
                    embedding.prev_in_adjacency_list(new_to_id, new_from_id, new_edge_id);
                const size_t old_prev_edge_id = get_edge_new_to_old_id().get_label(prev_edge.id);
                destination_embedding
                    .add_edge_after(old_to_id, old_from_id, old_edge_id, old_prev_edge_id);
            }
        }
    }
}

const Path* PlanarizedCylinder::non_repeated_path_in_component(
    const Graph& component, const Face& face, const EdgesLabels<size_t>& component_edge_labels
) {
    const size_t edge_id_non_repeated_path_0 = face.non_repeated_paths()[0].get_first_edge_id();
    const size_t edge_id_non_repeated_path_1 = face.non_repeated_paths()[1].get_first_edge_id();

    for (const EdgeId edge : component.get_all_edges()) {
        const size_t true_edge_id =
            edge_new_to_old_id.get_label(component_edge_labels.get_label(edge.id));
        if (true_edge_id == edge_id_non_repeated_path_0)
            return &face.non_repeated_paths()[0];
        if (true_edge_id == edge_id_non_repeated_path_1)
            return &face.non_repeated_paths()[1];
    }

    return nullptr;
}

bool PlanarizedCylinder::is_consistent(
    const Graph& component,
    const NodesLabels<size_t>& component_node_labels,
    const Path& non_repeated_path
) {
    const size_t old_node_id = non_repeated_path.get_first_node_id();
    const size_t old_first_edge_id = non_repeated_path.get_first_edge_id();
    const size_t old_last_edge_id = non_repeated_path.get_last_edge_id();

    for (const size_t n_id : component.get_nodes_ids()) {
        const size_t cylinder_n_id = component_node_labels.get_label(n_id);
        const size_t old_n_id = node_new_to_old_id.get_label(cylinder_n_id);
        if (old_node_id != old_n_id)
            continue;
        if (component.get_degree_of_node(n_id) == 2)
            return true;

        const size_t embedding_node_id = node_old_to_new_id.get_label(old_node_id);
        const size_t embedding_neighbor_id =
            node_old_to_new_id.get_label(non_repeated_path.get_node_id_at_position(1));
        const size_t embedding_first_edge_id = edge_old_to_new_id.get_label(old_first_edge_id);
        const size_t embedding_last_edge_id = edge_old_to_new_id.get_label(old_last_edge_id);

        return (
            embedding
                .next_in_adjacency_list(
                    embedding_node_id,
                    embedding_neighbor_id,
                    embedding_first_edge_id
                )
                .id == embedding_last_edge_id
        );
    }

    DOMUS_ASSERT(
        false,
        "PlanarizedCylinder::is_consistent: should have not reached this part of code."
    );
    return true;
}

// note that once we compute the PlanarizedCylinder, even if the procedure succedes, once obtained
// it might not be ready as it is to be used to emebed pieces inside the real cylinder. that is
// because the obtained rotation scheme of the PlanarizedCylinder might be such that stuff would
// acually end up being placed outside of it in the real cylinder (stuff with attachments in the
// non-repeated chains, in particular). before actually using the PlanarCylinder, we must check
// whether its embedding is consistent with the real cylinder we're working with. if it is not, then
// we have to reverse some vertices' circular orders, and then it will be good.

// to do this we check all of the at most 2 biconnected components in the planarized cylinder that
// contains edges from the non-repeated paths of the face. these components must contain at least
// one of the two endpoints of the repeated path of the cylinder. we simply need to check if that
// particular vertex has the same rotation scheme of the complete_graph_embedding. in case it does
// that particular involved biconnected component is consistent and nothing needs to be done.
// otherwise we have to reverse all circular orders of the vertex involved in that biconnected
// component.
void PlanarizedCylinder::adjust_rotation_scheme(const Face& face) {
    auto biconnected_components = BiconnectedComponents::compute(graph);
    for (size_t i = 0; i < biconnected_components.get_components().size(); i++) {
        const Graph& component = biconnected_components.get_components()[i];
        const auto& component_edge_labels = biconnected_components.get_edge_labels_of_component(i);

        // check if it contains at least one edge from one of the two non repeated paths
        const Path* non_repeated_path =
            non_repeated_path_in_component(component, face, component_edge_labels);

        if (non_repeated_path == nullptr)
            continue; // this biconnected component does not contain any non-repeated path

        if (is_consistent(
                component,
                biconnected_components.get_node_labels_of_component(i),
                *non_repeated_path
            ))
            continue;

        const auto& component_node_labels = biconnected_components.get_node_labels_of_component(i);
        for (size_t node_id : component.get_nodes_ids()) {
            const size_t cylinder_node_id = component_node_labels.get_label(node_id);
            embedding.reverse_circular_order(cylinder_node_id);
        }
    }
}

} // namespace domus::torus