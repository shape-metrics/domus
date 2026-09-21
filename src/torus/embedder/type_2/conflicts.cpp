#include "conflicts.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

#include "adjacencies.hpp"
#include "domus/core/debug.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/planarity/interlacement.hpp"
#include "domus/sat/cnf.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace sat::cnf;
using namespace sat;

struct Variable {
    size_t piece_index;
    size_t face_index;
};

Conflicts::Conflicts(const Adjacencies& adjacencies)
    : m_adjacencies(adjacencies), m_faces_conflicts(adjacencies.get_faces().size()) {
    for (size_t face_index = 0; face_index < adjacencies.get_faces().size(); face_index++) {
        const Face& face = adjacencies.get_faces()[face_index];
        if (face.type() == FaceType::TYPE_1)
            m_faces_conflicts[face_index] =
                SimpleFaceConflicts{conflicts_between_ordinary_pieces_in_face(face_index)};
        else
            conflicts_cylinder_face(face_index);
    }
}

const std::vector<Conflict>& Conflicts::simple_face_conflicts(size_t face_index) const {
    return std::get<SimpleFaceConflicts>(m_faces_conflicts[face_index]).conflicts;
}

std::vector<Conflict> Conflicts::conflicts_between_ordinary_pieces_in_face(size_t face_index) {
    const std::vector<size_t>& pieces = m_adjacencies.ordinary_pieces_in_face(face_index);
    if (pieces.size() < 2)
        return {};

    const Face& face = m_adjacencies.get_faces()[face_index];
    const size_t cycle_size = face.path().number_of_nodes() - 1;

    std::vector<size_t> node_to_cycle_pos(
        m_adjacencies.get_graph().get_number_of_nodes(),
        std::numeric_limits<size_t>::max()
    );
    for (size_t i = 0; i < cycle_size; ++i) {
        node_to_cycle_pos[face.path().get_node_id_at_position(i)] = i;
    }

    std::vector<std::vector<size_t>> piece_positions(pieces.size());
    for (size_t j = 0; j < pieces.size(); ++j) {
        const Bridge& bridge = m_adjacencies.get_pieces()[pieces[j]];
        piece_positions[j].reserve(bridge.number_of_attachments());
        for (const size_t old_att_id : bridge.get_old_attachments()) {
            const size_t cycle_pos = node_to_cycle_pos[old_att_id];
            DOMUS_ASSERT(
                cycle_pos != std::numeric_limits<size_t>::max(),
                "Conflicts::conflicts_between_ordinary_pieces_in_face: attachment node not found "
                "in face path"
            );
            piece_positions[j].push_back(cycle_pos);
        }
        std::ranges::sort(piece_positions[j]);
        auto [first, last] = std::ranges::unique(piece_positions[j]);
        piece_positions[j].erase(first, last);
        DOMUS_ASSERT(
            piece_positions[j].size() > 1,
            "Conflicts::conflicts_between_ordinary_pieces_in_face: piece has fewer than 2 "
            "attachments"
        );
    }

    std::vector<Conflict> conflicts;

    planarity::compute_conflicts(cycle_size, piece_positions, [&](size_t j1, size_t j2) {
        conflicts.push_back(Conflict{pieces[j1], pieces[j2]});
    });

    return conflicts;
}

std::vector<Conflict> Conflicts::conflicts_non_across_ordinary_pieces_VS_special_pieces(
    size_t face_index,
    const std::vector<size_t>& ordinary_non_across_pieces,
    const std::vector<size_t>& special_pieces
) {
    // to detect this kind of conflicts, we leverage the fact that the particular embedding of the
    // special pieces does not matter at all. meaning that, even tho the special pieces attachments
    // have multiple places to attach to, since they attach to the repeated paths of the cylinder,
    // we can randomly select the ones we want and it will work! only works because the ordinary
    // piece is non-across and the graph is cubic
    if (ordinary_non_across_pieces.empty() || special_pieces.empty())
        return {};

    const Face& face = m_adjacencies.get_faces()[face_index];
    const size_t cycle_size = face.path().number_of_nodes() - 1;

    std::vector<size_t> node_to_cycle_pos(
        m_adjacencies.get_graph().get_number_of_nodes(),
        std::numeric_limits<size_t>::max()
    );
    for (size_t i = 0; i < cycle_size; ++i) {
        const size_t node_id = face.path().get_node_id_at_position(i);
        if (node_to_cycle_pos[node_id] == std::numeric_limits<size_t>::max())
            node_to_cycle_pos[node_id] = i;
    }

    auto extract_positions = [&](const std::vector<size_t>& piece_indices) {
        std::vector<std::vector<size_t>> positions(piece_indices.size());
        for (size_t j = 0; j < piece_indices.size(); ++j) {
            const Bridge& bridge = m_adjacencies.get_pieces()[piece_indices[j]];
            positions[j].reserve(bridge.number_of_attachments());
            for (const size_t old_att_id : bridge.get_old_attachments()) {
                const size_t cycle_pos = node_to_cycle_pos[old_att_id];
                DOMUS_ASSERT(
                    cycle_pos != std::numeric_limits<size_t>::max(),
                    "Conflicts::conflicts_non_across_ordinary_pieces_VS_special_pieces: "
                    "attachment node not found in face path"
                );
                positions[j].push_back(cycle_pos);
            }
            std::ranges::sort(positions[j]);
            auto [first, last] = std::ranges::unique(positions[j]);
            positions[j].erase(first, last);
            DOMUS_ASSERT(
                positions[j].size() > 1,
                "Conflicts::conflicts_non_across_ordinary_pieces_VS_special_pieces: piece has "
                "fewer than 2 attachments"
            );
        }
        return positions;
    };

    const std::vector<std::vector<size_t>> ord_positions =
        extract_positions(ordinary_non_across_pieces);
    const std::vector<std::vector<size_t>> spec_positions = extract_positions(special_pieces);

    std::vector<Conflict> conflicts;
    for (size_t j = 0; j < ordinary_non_across_pieces.size(); ++j) {
        planarity::CycleConflictDetector detector(cycle_size, ord_positions[j]);
        for (size_t k = 0; k < special_pieces.size(); ++k) {
            if (detector.in_conflict(spec_positions[k]))
                conflicts.push_back(Conflict{ordinary_non_across_pieces[j], special_pieces[k]});
        }
    }

    return conflicts;
}

void Conflicts::conflicts_cylinder_face(size_t face_index) {
    CylinderConflicts conflicts;
    conflicts.ordinary_pieces = conflicts_between_ordinary_pieces_in_face(face_index);

    const std::vector<size_t>& special_pieces = m_adjacencies.special_pieces_in_face(face_index);

    std::vector<size_t> ordinary_across_pieces;
    std::vector<size_t> ordinary_non_across_pieces;
    for (size_t ordinary_piece : m_adjacencies.ordinary_pieces_in_face(face_index)) {
        if (m_adjacencies.is_ordinary_piece_cutting_cylinder(ordinary_piece, face_index))
            ordinary_across_pieces.push_back(ordinary_piece);
        else
            ordinary_non_across_pieces.push_back(ordinary_piece);
    }

    conflicts.non_across_ordinary_pieces_VS_special_pieces =
        conflicts_non_across_ordinary_pieces_VS_special_pieces(
            face_index,
            ordinary_non_across_pieces,
            special_pieces
        );

    // TODO detect these conflicts
    /*
    onesided_special_pieces;

    across_ordinary_pieces_VS_onesided_1_special_pieces;
    across_ordinary_pieces_VS_onesided_2_special_pieces;
    */

    m_faces_conflicts[face_index] = std::move(conflicts);
}

std::optional<const std::vector<size_t>>
Conflicts::solve(std::vector<CylinderType> cylinders_type_current_guess) const {
    return std::nullopt;
    // TODO
}

} // namespace domus::torus