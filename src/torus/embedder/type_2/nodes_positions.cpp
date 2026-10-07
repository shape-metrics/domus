#include "nodes_positions.hpp"
#include "domus/core/debug.hpp"

namespace domus::torus {

NodesPositions::NodesPositions(const Adjacencies& adjacencies) {
    const std::vector<Face>& faces = adjacencies.get_faces();
    std::vector<bool> is_attachment;
    for (const size_t piece_index : adjacencies.all_ordinary_pieces()) {
        const Bridge& piece = adjacencies.get_pieces()[piece_index];
        for (size_t attachment_id : piece.get_old_attachments()) {
            if (is_attachment.size() <= attachment_id)
                is_attachment.resize(attachment_id + 1, false);
            is_attachment[attachment_id] = true;
        }
    }
    for (size_t face_index = 0; face_index < faces.size(); ++face_index) {
        const Face& face = faces[face_index];
        for (size_t i = 0; i < face.path().number_of_edges(); ++i) {
            size_t node_id = face.path().get_node_id_at_position(i);
            if (node_id >= is_attachment.size() || !is_attachment[node_id])
                continue;
            if (!m_node_position.has_label(node_id))
                m_node_position.add_label(node_id, {std::nullopt, std::nullopt});
            auto& array = m_node_position.get_label(node_id);
            DOMUS_ASSERT(
                !array[0].has_value() || !array[1].has_value(),
                "NodesPositions::NodesPositions: both positions already set."
            );
            if (array[0].has_value())
                array[1] = {face_index, i};
            else
                array[0] = {face_index, i};
        }
    }
}

size_t NodesPositions::get_position_of_node_in_face(size_t node_id, size_t face_id) const {
    for (auto& opt : m_node_position.get_label(node_id))
        if (opt.has_value() && opt->face_index == face_id)
            return opt->position_in_face;
    DOMUS_ASSERT(false, "NodesPositions::get_position_of_node_in_face: should not reach this");
    return 0;
}

} // namespace domus::torus