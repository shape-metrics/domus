#include "type_2.hpp"

#include <iostream>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/torus/faces.hpp"

#include "../utils.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

std::vector<NodesContainer> compute_nodes_in_face(const std::vector<Face>& faces) {
    std::vector<NodesContainer> nodes_in_face;
    for (size_t i = 0; i < faces.size(); ++i) {
        const Face& face = faces[i];
        nodes_in_face.emplace_back();
        for (size_t j = 0; j < face.path().number_of_nodes() - 1; ++j) {
            const size_t node_id = face.path().node_id_at_position(j);
            if (!nodes_in_face[i].has_node(node_id))
                nodes_in_face[i].add_node(node_id);
        }
    }
    return nodes_in_face;
}

bool is_piece_in_face(const NodesContainer& nodes_in_face, const Bridge& bridge) {
    for (const size_t attachment_id : bridge.get_attachments()) {
        const size_t old_attachment_id = bridge.get_new_id_to_old_id().get_label(attachment_id);
        if (!nodes_in_face.has_node(old_attachment_id))
            return false;
    }
    return true;
}

std::vector<size_t> adjacent_faces(
    const Bridge& bridge,
    const std::vector<size_t> faces_indexes,
    const std::vector<NodesContainer>& nodes_in_faces
) {
    std::vector<size_t> adjacent_faces;
    for (const size_t face_index : faces_indexes)
        if (is_piece_in_face(nodes_in_faces[face_index], bridge))
            adjacent_faces.push_back(face_index);
    return adjacent_faces;
}

struct SpecialPiece {
    const size_t bridge_index;
    const size_t special_face_index;
};

struct OrdinaryPiece {
    const size_t bridge_index;
    const std::vector<size_t> ordinary_faces_indexes;
};

bool handle_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
    add_log_final_configuration(faces);
    DOMUS_ASSERT(faces.size() == 2, "handle_type_2: expected 2 faces");
    const std::vector<Bridge> bridges = Bridge::compute(graph, embedding);

    std::vector<size_t> type_1_faces_indexes;
    std::vector<size_t> type_2_faces_indexes;
    for (size_t i = 0; i < faces.size(); ++i) {
        if (faces[i].type() == FaceType::TYPE_1)
            type_1_faces_indexes.push_back(i);
        else
            type_2_faces_indexes.push_back(i);
    }

    // char c;
    // std::cin >> c;

    std::vector<NodesContainer> nodes_in_face = compute_nodes_in_face(faces);

    std::vector<SpecialPiece> special_pieces;
    std::vector<OrdinaryPiece> ordinary_pieces;
    for (size_t i = 0; i < bridges.size(); ++i) {
        std::vector<size_t> adjacent_special_faces_indexes =
            adjacent_faces(bridges[i], type_2_faces_indexes, nodes_in_face);
        std::vector<size_t> adjacent_ordinary_faces_indexes =
            adjacent_faces(bridges[i], type_1_faces_indexes, nodes_in_face);
        if (adjacent_special_faces_indexes.size() + adjacent_ordinary_faces_indexes.size() == 0)
            return false;
        // if (adjacent_special_faces_indexes.size() > 0 && adjacent_ordinary_faces_indexes.size() >
        // 0)
        //     return false;
        // if (adjacent_special_faces_indexes.size() > 1)
        //     return false;
        if (adjacent_special_faces_indexes.size() == 1) {
            special_pieces.emplace_back(i, adjacent_special_faces_indexes[0]);
            continue;
        }
        ordinary_pieces.emplace_back(i, std::move(adjacent_ordinary_faces_indexes));
    }
    // TODO
    return false;
}

} // namespace domus::torus
