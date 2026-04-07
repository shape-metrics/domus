#include "type_3.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/torus/bridge.hpp"

#include "faces.hpp"

namespace domus::torus {
using namespace domus::graph;
using graph::utilities::NodesLabels;

void handle_type_3(
    Graph& graph,
    const std::vector<Bridge>& bridges,
    Embedding& embedding,
    const graph::Path& face_1,
    const graph::Path& face_2,
    const FaceType face_1_type,
    const FaceType face_2_type
) {
    // NodesLabels<size_t> face_nodes_pos_matrix(embedding);
    // std::vector<size_t> face_nodes;
    // for (const size_t node_id : embedding.get_nodes_ids())
    //     if (embedding.get_degree_of_node(node_id) > 0) {
    //         face_nodes_pos_matrix.add_label(node_id,
    //         face_nodes_pos_matrix.get_number_of_labels()); face_nodes.push_back(node_id);
    //     }

    // std::vector<std::vector<bool>> tried_pair_of_face_nodes(
    //     face_nodes_pos_matrix.get_number_of_labels(),
    //     std::vector<bool>(face_nodes_pos_matrix.get_number_of_labels(), false)
    // );

    // for (const Path& repeated_path : face.repeated_paths()) {
    //     // TODO need to check if this is actually good to do, maybe a parallel
    //     // edge (subdivided so we dont have multiple edges) should be actually tried
    //     for (size_t i = 0; i < repeated_path.number_of_edges(); i++) {
    //         const size_t edge_id = repeated_path.edge_id_at_position(i);
    //         auto [node_id_1, node_id_2] = graph.get_edge(edge_id);
    //         tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(node_id_1)]
    //                                 [face_nodes_pos_matrix.get_label(node_id_2)] = true;
    //         tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(node_id_2)]
    //                                 [face_nodes_pos_matrix.get_label(node_id_1)] = true;
    //     }
    // }

    // // trying path actually in the graph
    // for (const Bridge& bridge : bridges) {
    //     if (bridge.get_bridge().get_number_of_nodes() == 2) {
    //         const Path path = path_of_chord(graph, bridge);
    //         try_face_splits_with_path(graph, path, emb_cpy, face);
    //         const size_t first = path.get_first_node_id();
    //         const size_t last = path.get_last_node_id();
    //         tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
    //                                 [face_nodes_pos_matrix.get_label(last)] = true;
    //         tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
    //                                 [face_nodes_pos_matrix.get_label(first)] = true;
    //     } else {
    //         auto paths = candidate_face_splitting_paths_in_graph(graph, bridge);
    //         for (const Path& path : paths) {
    //             try_face_splits_with_path(graph, path, emb_cpy, face);
    //             const size_t first = path.get_first_node_id();
    //             const size_t last = path.get_last_node_id();
    //             tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(first)]
    //                                     [face_nodes_pos_matrix.get_label(last)] = true;
    //             tried_pair_of_face_nodes[face_nodes_pos_matrix.get_label(last)]
    //                                     [face_nodes_pos_matrix.get_label(first)] = true;
    //         }
    //     }
    // }

    // // trying paths (edges) not in the graph
    // for (size_t i = 0; i < face_nodes.size() - 1; i++) {
    //     for (size_t j = i + 1; j < face_nodes.size(); j++) {
    //         if (tried_pair_of_face_nodes[i][j])
    //             continue;
    //         const size_t node_id_1 = face_nodes[i];
    //         const size_t node_id_2 = face_nodes[j];
    //         DOMUS_ASSERT(
    //             !graph.are_neighbors(node_id_1, node_id_2),
    //             "handle_type_4: candidate nodes {} {} should not be neighbors\n{}",
    //             node_id_1,
    //             node_id_2,
    //             embedding.to_string()
    //         );
    //         const size_t edge_id = graph.add_edge(node_id_1, node_id_2);
    //         Path path;
    //         path.push_back(graph, node_id_1, edge_id);
    //         try_face_splits_with_path(graph, path, emb_cpy, face);
    //         graph.remove_edge(edge_id);
    //     }
    // }
}

void handle_type_3(
    graph::Graph& graph,
    graph::Embedding& embedding,
    const graph::Path& face_1,
    const graph::Path& face_2,
    const FaceType face_1_type,
    const FaceType face_2_type
) {
    handle_type_3(
        graph,
        Bridge::compute(graph, embedding),
        embedding,
        face_1,
        face_2,
        face_1_type,
        face_2_type
    );
}

} // namespace domus::torus