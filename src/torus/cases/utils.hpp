#pragma once

#include <bitset>
#include <fstream>
#include <ranges>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"

#include "domus/torus/bridge.hpp"

#include "../faces.hpp"
#include "1/type_1.hpp"
#include "2/type_2.hpp"

namespace domus::torus {

inline auto all_pairs_of_view(auto&& feet) {
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

inline bool are_attachments_in_same_repeated_path(
    const graph::utilities::NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    const size_t attachment_id_1,
    const size_t attachment_id_2
) {
    const std::bitset<3>& repeated_paths_1 = is_node_in_repeated_path.get_label(attachment_id_1);
    const std::bitset<3>& repeated_paths_2 = is_node_in_repeated_path.get_label(attachment_id_2);
    for (size_t i = 0; i < 3; i++)
        if (repeated_paths_1.test(i) && repeated_paths_2.test(i))
            return true;
    return false;
}

inline graph::Path path_of_chord(const graph::Graph& graph, const Bridge& chord) {
    DOMUS_ASSERT(
        chord.get_bridge().get_number_of_nodes() == 2,
        "path_of_chord: function only for bridges that are a single edge"
    );
    const size_t node_1 = chord.get_new_id_to_old_id().get_label(0);
    const size_t edge_id = chord.get_new_edge_id_to_old_id().get_label(0);
    graph::Path path;
    path.push_back(graph, node_1, edge_id);
    return path;
}

inline void augment_embedding_with_path(graph::Embedding& embedding, const graph::Path& path) {
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

inline void
remove_augment_of_path_in_embedding(graph::Embedding& embedding, const graph::Path& path) {
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

inline bool next_case(graph::Embedding& embedding, graph::Graph& graph) {
    DOMUS_ASSERT(
        compute_embedding_genus(embedding) == 1,
        "Type4Handler::next_case: genus of embedding after edge insertions is not 1"
    );
    std::vector<Face> faces;
    for (graph::Path path : compute_faces_in_embedding(graph, embedding))
        faces.push_back(compute_face_from_path(std::move(path), graph));
    for (const Face& face : faces)
        if (face.type() == FaceType::TYPE_2)
            return handle_type_2(graph, embedding, faces);
    return handle_type_1(graph, embedding, faces);
}

static std::ofstream log_final_configurations("log_final_configurations.txt");
inline void add_log_final_configuration(const std::vector<domus::torus::Face>& faces) {
    if (log_final_configurations.is_open()) {
        std::vector<size_t> face_types;
        face_types.reserve(faces.size());

        for (auto& face : faces)
            face_types.push_back(static_cast<size_t>(face.type()));
        std::sort(face_types.begin(), face_types.end());

        for (auto& type : face_types)
            log_final_configurations << type << " ";
        log_final_configurations << std::endl;
    }
}

} // namespace domus::torus
