#include "type_4.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/torus/bridge.hpp"

#include "faces.hpp"
#include "next_type_establisher.hpp"
#include "splitting_paths.hpp"
#include "type_3.hpp"

namespace domus::torus {
using namespace domus::graph;
using domus::graph::utilities::NodesLabels;

void augment_embedding_with_path(Embedding& embedding, const Path& path) {
    for (size_t i = 1; i < path.number_of_edges() - 1; ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.add_edge(node_id_1, node_id_2, edge_id);
        embedding.add_edge(node_id_2, node_id_1, edge_id);
    }
    if (path.number_of_edges() > 1) {
        const size_t first_node_id = path.get_first_node_id();
        const size_t second_node_id = path.node_id_at_position(1);
        const size_t first_edge_id = path.edge_id_at_position(0);
        embedding.add_edge(second_node_id, first_node_id, first_edge_id);

        const size_t last_node_id = path.get_last_node_id();
        const size_t second_last_node_id = path.node_id_at_position(path.number_of_edges() - 1);
        const size_t last_edge_id = path.edge_id_at_position(path.number_of_edges() - 1);
        embedding.add_edge(second_last_node_id, last_node_id, last_edge_id);
    }
}

void remove_augment_of_path_in_embedding(Embedding& embedding, const Path& path) {
    for (size_t i = 1; i < path.number_of_edges() - 1; ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.remove_edge(node_id_1, node_id_2, edge_id);
        embedding.remove_edge(node_id_2, node_id_1, edge_id);
    }
    if (path.number_of_edges() > 1) {
        const size_t first_node_id = path.get_first_node_id();
        const size_t second_node_id = path.node_id_at_position(1);
        const size_t first_edge_id = path.edge_id_at_position(0);
        embedding.remove_edge(second_node_id, first_node_id, first_edge_id);

        const size_t last_node_id = path.get_last_node_id();
        const size_t second_last_node_id = path.node_id_at_position(path.number_of_edges() - 1);
        const size_t last_edge_id = path.edge_id_at_position(path.number_of_edges() - 1);
        embedding.remove_edge(second_last_node_id, last_node_id, last_edge_id);
    }
}

bool did_path_split(
    const Face& initial_face, const Path& path, const Path& face_1, const Path& face_2
) {
    const size_t first_of_path = path.get_first_node_id();
    const size_t last_of_path = path.get_last_node_id();

    const size_t first_repeated_id = initial_face.repeated_paths()[0].get_first_node_id();
    const size_t last_repeated_id = initial_face.repeated_paths()[0].get_last_node_id();

    if ((first_of_path == first_repeated_id && last_of_path == last_repeated_id) ||
        (first_of_path == last_repeated_id && last_of_path == first_repeated_id))
        return true;

    if (first_of_path == last_of_path)
        return true;

    const size_t first_count_1 = node_id_count_in_path(face_1, first_repeated_id);
    const size_t last_count_1 = node_id_count_in_path(face_1, last_repeated_id);
    if (first_count_1 == 3 && last_count_1 == 3)
        return false;

    const size_t first_count_2 = node_id_count_in_path(face_2, first_repeated_id);
    const size_t last_count_2 = node_id_count_in_path(face_2, last_repeated_id);
    if (first_count_2 == 3 && last_count_2 == 3)
        return false;

    if (first_count_1 < 3 && last_count_1 < 3 && first_count_2 < 3 && last_count_2 < 3)
        return true;

    if (first_of_path == first_repeated_id || first_of_path == last_repeated_id)
        return true;
    if (last_of_path == first_repeated_id || last_of_path == last_repeated_id)
        return true;

    return false;
}

void try_face_splits_with_path(
    Graph& graph, const Path& path, Embedding& embedding, const Face& face
) {
    const size_t first_id = path.get_first_node_id();
    const size_t last_id = path.get_last_node_id();
    const size_t n_edges = path.number_of_edges();

    DOMUS_ASSERT(n_edges > 0, "try_face_splits_with_path: empty path");
    DOMUS_ASSERT(
        embedding.get_degree_of_node(first_id) > 1,
        "try_face_splits_with_path: first node degree <= 1"
    );
    DOMUS_ASSERT(
        embedding.get_degree_of_node(last_id) > 1,
        "try_face_splits_with_path: last node degree <= 1"
    );

    std::vector<EdgeIter> first_edges;
    for (const EdgeIter e : embedding.get_edges(first_id))
        first_edges.push_back(e);

    std::vector<EdgeIter> last_edges;
    for (const EdgeIter e : embedding.get_edges(last_id))
        last_edges.push_back(e);

    augment_embedding_with_path(embedding, path);

    const size_t second_id = path.node_id_at_position(1);
    const size_t second_last_id = path.node_id_at_position(n_edges - 1);
    const size_t first_edge_id = path.edge_id_at_position(0);
    const size_t last_edge_id = path.edge_id_at_position(n_edges - 1);

    for (const EdgeIter edge1 : first_edges) {
        for (const EdgeIter edge2 : last_edges) {
            embedding.add_edge_after(first_id, second_id, first_edge_id, edge1.id);
            embedding.add_edge_after(last_id, second_last_id, last_edge_id, edge2.id);

            std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
            DOMUS_ASSERT(
                faces.size() == 2,
                "did_path_split: embedding should have exactly 2 faces"
            );

            if (!did_path_split(face, path, faces[0], faces[1])) {
                embedding.remove_edge(first_id, second_id, first_edge_id);
                embedding.remove_edge(last_id, second_last_id, last_edge_id);
                continue;
            }

            auto [face_1, face_2] =
                NextTypesEstablisher::compute_face_types(graph, path, face, faces[0], faces[1]);

            if (face_1.type() == FaceType::TYPE_3 || face_2.type() == FaceType::TYPE_3) {
                handle_type_3(graph, embedding, face_1, face_2);
            } else if (face_1.type() == FaceType::TYPE_2 || face_2.type() == FaceType::TYPE_2) {
                // TODO
            } else {
                DOMUS_ASSERT(
                    (face_1.type() == FaceType::TYPE_1 && face_2.type() == FaceType::TYPE_1),
                    "try_face_splits_with_path: outcome of faces types is invalid"
                );
                // TODO
            }

            embedding.remove_edge(first_id, second_id, first_edge_id);
            embedding.remove_edge(last_id, second_last_id, last_edge_id);
        }
    }

    remove_augment_of_path_in_embedding(embedding, path);
}

void handle_type_4(
    Graph& graph, const std::vector<Bridge>& bridges, const Embedding& embedding, const Face& face
) {
    Embedding emb_cpy = embedding;

    NodesLabels<size_t> face_nodes_pos_matrix(embedding);
    std::vector<size_t> face_nodes;
    for (const size_t node_id : embedding.get_nodes_ids())
        if (embedding.get_degree_of_node(node_id) > 0) {
            face_nodes_pos_matrix.add_label(node_id, face_nodes_pos_matrix.get_number_of_labels());
            face_nodes.push_back(node_id);
        }

    std::vector<std::vector<bool>> tried_pair_of_face_nodes(
        face_nodes_pos_matrix.get_number_of_labels(),
        std::vector<bool>(face_nodes_pos_matrix.get_number_of_labels(), false)
    );

    for (const Path& repeated_path : face.repeated_paths()) {
        // TODO need to check if this is actually good to do, maybe a parallel
        // edge (subdivided so we dont have multiple edges) should be actually tried
        for (size_t i = 0; i < repeated_path.number_of_edges(); i++) {
            const size_t edge_id = repeated_path.edge_id_at_position(i);
            auto [node_id_1, node_id_2] = graph.get_edge(edge_id);
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(node_id_1)]
                                    [face_nodes_pos_matrix.get_label(node_id_2)] = true;
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(node_id_2)]
                                    [face_nodes_pos_matrix.get_label(node_id_1)] = true;
        }
    }

    // trying path actually in the graph
    for (const Bridge& bridge : bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(graph, bridge);
            try_face_splits_with_path(graph, path, emb_cpy, face);
            const size_t first = path.get_first_node_id();
            const size_t last = path.get_last_node_id();
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
                                    [face_nodes_pos_matrix.get_label(last)] = true;
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
                                    [face_nodes_pos_matrix.get_label(first)] = true;
        } else {
            auto paths = candidate_face_splitting_paths_in_bridge(graph, bridge);
            for (const Path& path : paths) {
                try_face_splits_with_path(graph, path, emb_cpy, face);
                const size_t first = path.get_first_node_id();
                const size_t last = path.get_last_node_id();
                tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
                                        [face_nodes_pos_matrix.get_label(last)] = true;
                tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
                                        [face_nodes_pos_matrix.get_label(first)] = true;
            }
        }
    }

    // trying paths (edges) not in the graph
    for (size_t i = 0; i < face_nodes.size() - 1; i++) {
        for (size_t j = i + 1; j < face_nodes.size(); j++) {
            if (tried_pair_of_face_nodes[i][j])
                continue;
            const size_t node_id_1 = face_nodes[i];
            const size_t node_id_2 = face_nodes[j];
            DOMUS_ASSERT(
                !graph.are_neighbors(node_id_1, node_id_2),
                "handle_type_4: candidate nodes {} {} should not be neighbors\n{}",
                node_id_1,
                node_id_2,
                embedding.to_string()
            );
            const size_t edge_id = graph.add_edge(node_id_1, node_id_2);
            Path path;
            path.push_back(graph, node_id_1, edge_id);
            try_face_splits_with_path(graph, path, emb_cpy, face);
            graph.remove_edge(edge_id);
        }
    }
}

void handle_type_4(Graph& graph, const Embedding& embedding, const Face& face) {
    handle_type_4(graph, Bridge::compute(graph, embedding), embedding, face);
}

} // namespace domus::torus
