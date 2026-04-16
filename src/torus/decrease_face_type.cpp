#include "decrease_face_type.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/torus/bridge.hpp"

#include "faces.hpp"

namespace domus::torus {
using namespace domus::graph;
using graph::utilities::NodesContainer;
using graph::utilities::NodesLabels;

auto all_pairs_of_view(auto&& feet) {
    auto feet_view = std::views::all(std::forward<decltype(feet)>(feet));
    return feet_view | std::views::enumerate |
           std::views::transform([feet_view](auto&& tuple) mutable {
               auto [i, first] = tuple;
               // for each element at i, create pairs with elements from i+1 to end
               return feet_view | std::views::drop(i + 1) |
                      std::views::transform([first](auto&& second) {
                          return std::make_pair(first, second);
                      });
           }) |
           std::views::join; // flatten
}

auto candidate_face_splitting_paths_in_bridge(const graph::Graph& graph, const Bridge& bridge) {
    return all_pairs_of_view(compute_all_feet_in_bridge(bridge)) |
           std::views::transform([&](const auto& pair) {
               const graph::EdgeId& foot_1 = pair.first;
               const graph::EdgeId& foot_2 = pair.second;
               const size_t from_id_1 = foot_1.edge.from_id;
               const size_t to_id_1 = foot_1.edge.to_id;
               const size_t attachment_1 = (bridge.is_attachment(from_id_1)) ? from_id_1 : to_id_1;
               const size_t inner_1 = (attachment_1 == from_id_1) ? to_id_1 : from_id_1;

               const size_t from_id_2 = foot_2.edge.from_id;
               const size_t to_id_2 = foot_2.edge.to_id;
               const size_t attachment_2 = (bridge.is_attachment(from_id_2)) ? from_id_2 : to_id_2;
               const size_t inner_2 = (attachment_2 == from_id_2) ? to_id_2 : from_id_2;

               graph::Path path = (inner_1 != inner_2)
                                      ? graph::algorithms::find_shortest_path_between_nodes(
                                            bridge.get_bridge(),
                                            inner_1,
                                            inner_2
                                        )
                                            .value()
                                      : graph::Path();
               path.push_front(bridge.get_bridge(), inner_1, foot_1.id);
               path.push_back(bridge.get_bridge(), inner_2, foot_2.id);
               return path;
           }) |
           std::views::transform([&](const graph::Path& path) {
               graph::Path old_path;
               for (const auto [edge_id, prev_node_id] : path.get_edges()) {
                   const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(edge_id);
                   const size_t old_prev_node_id =
                       bridge.get_new_id_to_old_id().get_label(prev_node_id);
                   old_path.push_back(graph, old_prev_node_id, old_edge_id);
               }
               return old_path;
           });
}

Path path_of_chord(const Graph& graph, const Bridge& chord) {
    DOMUS_ASSERT(
        chord.get_bridge().get_number_of_nodes() == 2,
        "path_of_chord: function only for bridges that are a single edge"
    );
    const size_t node_1 = chord.get_new_id_to_old_id().get_label(0);
    const size_t edge_id = chord.get_new_edge_id_to_old_id().get_label(0);
    Path path;
    path.push_back(graph, node_1, edge_id);
    return path;
}

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

Face& highest_face_type(std::vector<Face>& faces) {
    Face* max_face = &faces[0];
    for (size_t i = 1; i < faces.size(); i++)
        if (faces[i].type() > max_face->type())
            max_face = &faces[i];
    return *max_face;
}

void try_path_insertion(Graph& graph, Embedding& embedding, const Face& face) {
    std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);

    if (faces.size() <= 2 && face.type() == FaceType::TYPE_3)
        return;

    std::vector<Face> faces_types;
    for (const graph::Path& f : faces)
        faces_types.push_back(compute_face_from_path(f, graph));
    for (const Face& f : faces_types)
        if (f.type() >= face.type()) // the path did not split the face
            return;

    Face& face_max_type = highest_face_type(faces_types);

    if (face_max_type.type() == FaceType::TYPE_3) {
        decrease_face_type(graph, embedding, face_max_type);
    } else if (face_max_type.type() == FaceType::TYPE_2) {
        // TODO
    } else {
        DOMUS_ASSERT(
            face_max_type.type() == FaceType::TYPE_1,
            "try_face_splits_with_path: outcome of faces types is invalid"
        );
        // TODO
    }
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

            try_path_insertion(graph, embedding, face);

            embedding.remove_edge(first_id, second_id, first_edge_id);
            embedding.remove_edge(last_id, second_last_id, last_edge_id);
        }
    }

    remove_augment_of_path_in_embedding(embedding, path);
}

bool is_bridge_in_face(const Bridge& bridge, const NodesContainer& nodes_in_face) {
    const NodesLabels<size_t>& old_labels = bridge.get_new_id_to_old_id();
    for (const size_t attachment_id : bridge.get_attachments()) {
        const size_t old_attachment_id = old_labels.get_label(attachment_id);
        if (!nodes_in_face.has_node(old_attachment_id))
            return false;
    }
    return true;
}

void try_paths_inside_graph(
    Graph& graph,
    const std::vector<Bridge>& bridges,
    Embedding& embedding,
    const Face& face,
    const NodesLabels<size_t>& face_nodes_pos_matrix,
    std::vector<std::vector<bool>>& tried_pair_of_face_nodes,
    const NodesContainer& nodes_in_face
) {
    for (const Bridge& bridge : bridges) {
        if (face.type() == FaceType::TYPE_3 && !is_bridge_in_face(bridge, nodes_in_face))
            continue;
        DOMUS_ASSERT(
            face.type() == FaceType::TYPE_3 || is_bridge_in_face(bridge, nodes_in_face),
            "decrease_face_type: if face is of type 4 the bridge has to be incident to it"
        );
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(graph, bridge);
            try_face_splits_with_path(graph, path, embedding, face);
            const size_t first = path.get_first_node_id();
            const size_t last = path.get_last_node_id();
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
                                    [face_nodes_pos_matrix.get_label(last)] = true;
            tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
                                    [face_nodes_pos_matrix.get_label(first)] = true;
        } else {
            auto paths = candidate_face_splitting_paths_in_bridge(graph, bridge);
            for (const Path& path : paths) {
                try_face_splits_with_path(graph, path, embedding, face);
                const size_t first = path.get_first_node_id();
                const size_t last = path.get_last_node_id();
                tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
                                        [face_nodes_pos_matrix.get_label(last)] = true;
                tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
                                        [face_nodes_pos_matrix.get_label(first)] = true;
            }
        }
    }
}

void try_edges_not_in_graph(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    const std::vector<std::vector<bool>>& tried_pair_of_face_nodes,
    const std::vector<size_t>& face_nodes
) {
    for (size_t i = 0; i < face_nodes.size() - 1; i++) {
        for (size_t j = i + 1; j < face_nodes.size(); j++) {
            if (tried_pair_of_face_nodes[i][j])
                continue;
            const size_t node_id_1 = face_nodes[i];
            const size_t node_id_2 = face_nodes[j];
            if (graph.are_neighbors(node_id_1, node_id_2))
                continue;
            const size_t edge_id = graph.add_edge(node_id_1, node_id_2);
            Path path;
            path.push_back(graph, node_id_1, edge_id);
            try_face_splits_with_path(graph, path, embedding, face);
            graph.remove_edge(edge_id);
        }
    }
}

void decrease_face_type(
    Graph& graph, const std::vector<Bridge>& bridges, Embedding& embedding, const Face& face
) {
    NodesContainer nodes_in_face(graph);
    for (const size_t node_id : graph.get_nodes_ids()) {
        nodes_in_face.add_node(node_id);
    }

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
    try_paths_inside_graph(
        graph,
        bridges,
        embedding,
        face,
        face_nodes_pos_matrix,
        tried_pair_of_face_nodes,
        nodes_in_face
    );

    // trying paths (edges) not in the graph
    try_edges_not_in_graph(graph, embedding, face, tried_pair_of_face_nodes, face_nodes);
}

void decrease_face_type(Graph& graph, Embedding& embedding, const Face& face) {
    decrease_face_type(graph, Bridge::compute(graph, embedding), embedding, face);
}

} // namespace domus::torus
