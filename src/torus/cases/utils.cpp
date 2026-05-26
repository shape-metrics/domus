#include "utils.hpp"

#include <fstream>

#include "../draw.hpp"
#include "1/type_1.hpp"
#include "2/type_2.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

bool are_attachments_in_same_repeated_path(
    const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
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

bool next_case(Embedding& embedding, Graph& graph, const Face& original_face) {
    DOMUS_ASSERT(compute_embedding_genus(embedding) == 1, "next_case: genus of embedding is not 1");
    std::vector<Face> faces;
    for (Path path : compute_faces_in_embedding(graph, embedding))
        faces.push_back(compute_face_from_path(std::move(path), graph));
    for (const Face& face : faces)
        if (face.type() == FaceType::TYPE_2)
            return handle_type_2(graph, embedding, faces);
    return handle_type_1(graph, embedding, faces);
}

static std::ofstream log_final_configurations("log_final_configurations.txt");
void add_log_final_configuration(const std::vector<Face>& faces) {
    if (log_final_configurations.is_open()) {
        std::vector<size_t> face_types;
        face_types.reserve(faces.size());

        for (const Face& face : faces)
            face_types.push_back(static_cast<size_t>(face.type()));
        std::sort(face_types.begin(), face_types.end());

        for (size_t type : face_types)
            log_final_configurations << type << " ";
        log_final_configurations << std::endl;
    }
}

bool is_initial_face_valid(const Face& face) {
    if (face.repeated_paths().size() == 0)
        return true;
    const Path& rep_1 = face.repeated_paths()[0];
    for (size_t i = 1; i < face.repeated_paths().size(); ++i) {
        const Path& other_rep = face.repeated_paths()[i];
        if (rep_1.get_first_node_id() != other_rep.get_first_node_id())
            return false;
        if (rep_1.get_last_node_id() != other_rep.get_last_node_id())
            return false;
    }
    return true;
}

NodesLabels<std::bitset<3>> compute_nodes_in_repeated_paths(const Graph& graph, const Face& face) {
    NodesLabels<std::bitset<3>> is_node_in_repeated_path(graph);
    for (const size_t node_id : graph.get_nodes_ids())
        is_node_in_repeated_path.add_label(node_id, {});
    for (size_t i = 0; i < face.repeated_paths().size(); i++) {
        const Path& repeated_path = face.repeated_paths()[i];
        for (size_t j = 1; j < repeated_path.number_of_nodes() - 1; j++) {
            const size_t node_id = repeated_path.node_id_at_position(j);
            is_node_in_repeated_path.get_label(node_id).set(i);
        }
    }
    const size_t first_id = face.repeated_paths()[0].get_first_node_id();
    const size_t last_id = face.repeated_paths()[0].get_last_node_id();
    is_node_in_repeated_path.get_label(first_id).set();
    is_node_in_repeated_path.get_label(last_id).set();
    return is_node_in_repeated_path;
}

} // namespace domus::torus
