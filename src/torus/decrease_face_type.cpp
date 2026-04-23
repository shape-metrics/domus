#include "decrease_face_type.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/torus/bridge.hpp"

#include "faces.hpp"

// #include <fstream>
// static std::ofstream log_final_configurations("log_final_configurations.txt");
// void add_log_final_configuration(std::vector<domus::torus::Face>& faces) {
//     if (log_final_configurations.is_open()) {
//         std::vector<size_t> face_types;
//         face_types.reserve(faces.size());

//         for (auto& face : faces)
//             face_types.push_back(static_cast<size_t>(face.type()));
//         std::sort(face_types.begin(), face_types.end());

//         for (auto& type : face_types)
//             log_final_configurations << type << " ";
//         log_final_configurations << std::endl;
//     }
// }

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

void try_path_insertion(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
    std::vector<Face> faces_types;
    faces_types.reserve(faces.size());

    for (graph::Path& f : faces) {
        faces_types.push_back(compute_face_from_path(std::move(f), graph));
        if (faces_types.back().type() >= face.type()) // the path did not split the face
            return;
    }

    Face& face_max_type = highest_face_type(faces_types);

    if (face_max_type.type() == FaceType::TYPE_3) {
        decrease_face_type(graph, embedding, face_max_type, jolly_id_1, jolly_id_2);
    } else if (face_max_type.type() == FaceType::TYPE_2) {
        // add_log_final_configuration(faces_types);
        // TODO
    } else {
        // add_log_final_configuration(faces_types);
        DOMUS_ASSERT(
            face_max_type.type() == FaceType::TYPE_1,
            "try_face_splits_with_path: outcome of faces types is invalid"
        );
        // TODO
    }
}

// TODO: check if this function is correct
auto possible_insertions_of_path(const Path& path, const Face& face) {
    const size_t first_id = path.get_first_node_id();
    const size_t last_id = path.get_last_node_id();
    std::vector<size_t> first_candidate_insertions;
    std::vector<size_t> last_candidate_insertions;
    first_candidate_insertions.reserve(face.path().number_of_edges());
    last_candidate_insertions.reserve(face.path().number_of_edges());

    for (size_t i = 0; i < face.path().number_of_edges(); i++) {
        const size_t node_id = face.path().node_id_at_position(i + 1);
        const size_t edge_id = face.path().edge_id_at_position(i);
        if (node_id == first_id)
            first_candidate_insertions.push_back(edge_id);
        if (node_id == last_id)
            last_candidate_insertions.push_back(edge_id);
    }
    return std::make_pair(
        std::move(first_candidate_insertions),
        std::move(last_candidate_insertions)
    );
}

void try_face_splits_with_path(
    Graph& graph,
    const Path& path,
    Embedding& embedding,
    const Face& face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    DOMUS_ASSERT(
        compute_embedding_genus(embedding) == 1,
        "try_face_splits_with_path: initial genus of embedding is not 1"
    );
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

    auto [first_edges, last_edges] = possible_insertions_of_path(path, face);

    augment_embedding_with_path(embedding, path);

    const size_t second_id = path.node_id_at_position(1);
    const size_t second_last_id = path.node_id_at_position(n_edges - 1);
    const size_t first_edge_id = path.edge_id_at_position(0);
    const size_t last_edge_id = path.edge_id_at_position(n_edges - 1);

    if (first_id != last_id) {
        for (const size_t edge_1_id : first_edges) {
            for (const size_t edge_2_id : last_edges) {
                embedding.add_edge_after(first_id, second_id, first_edge_id, edge_1_id);
                embedding.add_edge_after(last_id, second_last_id, last_edge_id, edge_2_id);

                DOMUS_ASSERT(
                    compute_embedding_genus(embedding) == 1,
                    "try_face_splits_with_path: new genus of embedding after edge insertions "
                    "is "
                    "not 1"
                );

                try_path_insertion(graph, embedding, face, jolly_id_1, jolly_id_2);

                embedding.remove_edge(first_id, second_id, first_edge_id);
                embedding.remove_edge(last_id, second_last_id, last_edge_id);
            }
        }
    } else {
        for (size_t i = 0; i < first_edges.size() - 1; i++) {
            const size_t edge_1_id = first_edges[i];
            for (size_t j = i + 1; j < last_edges.size(); j++) {
                const size_t edge_2_id = last_edges[j];

                embedding.add_edge_after(first_id, second_id, first_edge_id, edge_1_id);
                embedding.add_edge_after(last_id, second_last_id, last_edge_id, edge_2_id);

                DOMUS_ASSERT(
                    compute_embedding_genus(embedding) == 1,
                    "try_face_splits_with_path: new genus of embedding after edge insertions "
                    "is "
                    "not 1"
                );

                try_path_insertion(graph, embedding, face, jolly_id_1, jolly_id_2);

                embedding.remove_edge(first_id, second_id, first_edge_id);
                embedding.remove_edge(last_id, second_last_id, last_edge_id);
            }
        }
    }

    remove_augment_of_path_in_embedding(embedding, path);
}

bool is_bridge_in_face(const Bridge& bridge, const NodesContainer& is_attachment_in_face) {
    const NodesLabels<size_t>& old_labels = bridge.get_new_id_to_old_id();
    for (const size_t attachment_id : bridge.get_attachments()) {
        const size_t old_attachment_id = old_labels.get_label(attachment_id);
        if (!is_attachment_in_face.has_node(old_attachment_id))
            return false;
    }
    return true;
}

void try_paths_inside_graph(
    Graph& graph,
    const std::vector<Bridge>& bridges,
    Embedding& embedding,
    const Face& face,
    const NodesContainer& is_attachment_in_face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    for (const Bridge& bridge : bridges) {
        if (face.type() == FaceType::TYPE_3 && !is_bridge_in_face(bridge, is_attachment_in_face))
            continue;
        DOMUS_ASSERT(
            face.type() == FaceType::TYPE_3 || is_bridge_in_face(bridge, is_attachment_in_face),
            "decrease_face_type: if face is of type 4 the bridge has to be incident to it"
        );
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(graph, bridge);
            try_face_splits_with_path(graph, path, embedding, face, jolly_id_1, jolly_id_2);
        } else {
            auto paths = candidate_face_splitting_paths_in_bridge(graph, bridge);
            for (const Path& path : paths)
                try_face_splits_with_path(graph, path, embedding, face, jolly_id_1, jolly_id_2);
        }
    }
}

void try_edges_not_in_graph(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    const std::vector<size_t>& attachments_in_face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    for (size_t i = 0; i < attachments_in_face.size() - 1; i++) {
        const size_t node_id_1 = attachments_in_face[i];
        for (size_t j = i + 1; j < attachments_in_face.size(); j++) {
            const size_t node_id_2 = attachments_in_face[j];

            const size_t in_between_id =
                (graph.get_degree_of_node(jolly_id_1) == 0) ? jolly_id_1 : jolly_id_2;
            DOMUS_ASSERT(
                graph.get_degree_of_node(in_between_id) == 0,
                "try_edges_not_in_graph: no jolly node available"
            );

            const size_t edge_id_1 = graph.add_edge(node_id_1, in_between_id);
            const size_t edge_id_2 = graph.add_edge(in_between_id, node_id_2);

            Path path;
            path.push_back(graph, node_id_1, edge_id_1);
            path.push_back(graph, in_between_id, edge_id_2);
            try_face_splits_with_path(graph, path, embedding, face, jolly_id_1, jolly_id_2);

            graph.remove_edge(edge_id_1);
            graph.remove_edge(edge_id_2);
        }
    }
}

void decrease_face_type(
    Graph& graph,
    const std::vector<Bridge>& bridges,
    Embedding& embedding,
    const Face& face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    NodesContainer is_attachments_in_face(graph);
    std::vector<size_t> attachments_in_face;
    attachments_in_face.reserve(bridges.size() * 2);

    for (const Bridge& bridge : bridges) {
        const NodesLabels<size_t>& labels = bridge.get_new_id_to_old_id();
        for (const size_t attachment_id : bridge.get_attachments()) {
            const size_t old_id = labels.get_label(attachment_id);
            if (!is_attachments_in_face.has_node(old_id)) {
                is_attachments_in_face.add_node(old_id);
                attachments_in_face.push_back(old_id);
            }
        }
    }

    // trying path actually in the graph
    try_paths_inside_graph(
        graph,
        bridges,
        embedding,
        face,
        is_attachments_in_face,
        jolly_id_1,
        jolly_id_2
    );

    // trying paths (edges) not in the graph
    try_edges_not_in_graph(graph, embedding, face, attachments_in_face, jolly_id_1, jolly_id_2);
}

void decrease_face_type(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    const size_t jolly_id_1,
    const size_t jolly_id_2
) {
    decrease_face_type(
        graph,
        Bridge::compute(graph, embedding),
        embedding,
        face,
        jolly_id_1,
        jolly_id_2
    );
}

} // namespace domus::torus
