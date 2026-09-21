#include "conflicts.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <vector>

#include "adjacencies.hpp"
#include "domus/core/debug.hpp"
#include "domus/planarity/interlacement.hpp"
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace sat::cnf;
using namespace sat;

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

std::vector<size_t> Conflicts::conflicts_non_across_ordinary_pieces(
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

    std::vector<size_t> conflicts;
    for (size_t j = 0; j < ordinary_non_across_pieces.size(); ++j) {
        planarity::CycleConflictDetector detector(cycle_size, ord_positions[j]);
        for (size_t k = 0; k < special_pieces.size(); ++k)
            if (detector.in_conflict(spec_positions[k]))
                conflicts.push_back(ordinary_non_across_pieces[j]);
    }

    return conflicts;
}

struct CylinderNodePositions {
    std::vector<size_t> node_to_down_pos;
    std::vector<size_t> node_to_up_pos;
};

std::vector<size_t>
extract_piece_positions(const Bridge& bridge, const std::vector<size_t>& mapping) {
    std::vector<size_t> pos;
    pos.reserve(bridge.number_of_attachments());
    for (const size_t old_att_id : bridge.get_old_attachments()) {
        const size_t cycle_pos = mapping[old_att_id];
        DOMUS_ASSERT(
            cycle_pos != std::numeric_limits<size_t>::max(),
            "Conflicts: attachment node not found in face path"
        );
        pos.push_back(cycle_pos);
    }
    std::ranges::sort(pos);
    auto [first, last] = std::ranges::unique(pos);
    pos.erase(first, last);
    DOMUS_ASSERT(pos.size() > 1, "Conflicts: piece has fewer than 2 attachments");
    return pos;
}

CylinderNodePositions
compute_cylinder_node_positions(const Adjacencies& adjacencies, size_t face_index) {
    const Face& face = adjacencies.get_faces()[face_index];
    DOMUS_ASSERT(
        face.repeated_paths().size() == 1,
        "Conflicts::compute_cylinder_node_positions: expected exactly 1 repeated path"
    );
    const Path& repeated_path = face.repeated_paths()[0];
    const size_t R_len = repeated_path.number_of_nodes();
    DOMUS_ASSERT(
        R_len >= 2,
        "Conflicts::compute_cylinder_node_positions: repeated path has fewer than 2 nodes"
    );

    std::vector<size_t> node_to_R_idx(
        adjacencies.get_graph().get_number_of_nodes(),
        std::numeric_limits<size_t>::max()
    );
    for (size_t k = 0; k < R_len; ++k) {
        node_to_R_idx[repeated_path.get_node_id_at_position(k)] = k;
    }

    const Path& path = face.path();
    const size_t cycle_size = path.number_of_nodes() - 1;

    CylinderNodePositions positions{
        .node_to_down_pos = std::vector<size_t>(
            adjacencies.get_graph().get_number_of_nodes(),
            std::numeric_limits<size_t>::max()
        ),
        .node_to_up_pos = std::vector<size_t>(
            adjacencies.get_graph().get_number_of_nodes(),
            std::numeric_limits<size_t>::max()
        )
    };

    for (size_t i = 0; i < cycle_size; ++i) {
        const size_t u = path.get_node_id_at_position(i);
        const size_t r_idx = node_to_R_idx[u];
        if (r_idx == std::numeric_limits<size_t>::max()) {
            positions.node_to_down_pos[u] = i;
            positions.node_to_up_pos[u] = i;
        } else {
            bool is_down = false;
            if (r_idx + 1 < R_len) {
                const size_t next_u = path.get_node_id_at_position(i + 1);
                if (next_u == repeated_path.get_node_id_at_position(r_idx + 1))
                    is_down = true;
            } else {
                const size_t prev_u = (i == 0) ? path.get_node_id_at_position(cycle_size - 1)
                                               : path.get_node_id_at_position(i - 1);
                if (prev_u == repeated_path.get_node_id_at_position(r_idx - 1))
                    is_down = true;
            }

            if (is_down)
                positions.node_to_down_pos[u] = i;
            else
                positions.node_to_up_pos[u] = i;
        }
    }

    return positions;
}

std::vector<ConflictOneSided> Conflicts::conflicts_onesided_special_pieces(
    size_t face_index, const std::vector<size_t>& special_pieces
) {
    if (special_pieces.size() < 2)
        return {};

    const Face& face = m_adjacencies.get_faces()[face_index];
    const size_t cycle_size = face.path().number_of_nodes() - 1;
    const CylinderNodePositions node_positions =
        compute_cylinder_node_positions(m_adjacencies, face_index);

    std::vector<std::vector<size_t>> special_down_positions(special_pieces.size());
    std::vector<std::vector<size_t>> special_up_positions(special_pieces.size());
    for (size_t k = 0; k < special_pieces.size(); ++k) {
        const Bridge& bridge = m_adjacencies.get_pieces()[special_pieces[k]];
        special_down_positions[k] =
            extract_piece_positions(bridge, node_positions.node_to_down_pos);
        special_up_positions[k] = extract_piece_positions(bridge, node_positions.node_to_up_pos);
    }

    std::vector<ConflictOneSided> conflicts;
    for (size_t j1 = 0; j1 < special_pieces.size() - 1; ++j1) {
        planarity::CycleConflictDetector detector_down(cycle_size, special_down_positions[j1]);
        planarity::CycleConflictDetector detector_up(cycle_size, special_up_positions[j1]);

        for (size_t j2 = j1 + 1; j2 < special_pieces.size(); ++j2) {
            const size_t s1 = special_pieces[j1];
            const size_t s2 = special_pieces[j2];

            // S1 DOWN vs S2 DOWN
            if (detector_down.in_conflict(special_down_positions[j2])) {
                conflicts.push_back(
                    ConflictOneSided{s1, OneSidedType::DOWN, s2, OneSidedType::DOWN}
                );
            }
            // S1 UP vs S2 UP
            if (detector_up.in_conflict(special_up_positions[j2])) {
                conflicts.push_back(ConflictOneSided{s1, OneSidedType::UP, s2, OneSidedType::UP});
            }
            // S1 DOWN vs S2 UP
            if (detector_down.in_conflict(special_up_positions[j2])) {
                conflicts.push_back(ConflictOneSided{s1, OneSidedType::DOWN, s2, OneSidedType::UP});
            }
            // S1 UP vs S2 DOWN
            if (detector_up.in_conflict(special_down_positions[j2])) {
                conflicts.push_back(ConflictOneSided{s1, OneSidedType::UP, s2, OneSidedType::DOWN});
            }
        }
    }

    return conflicts;
}

std::vector<Conflict> Conflicts::conflicts_across_ordinary_pieces_VS_onesided_special_pieces(
    size_t face_index,
    const std::vector<size_t>& ordinary_across_pieces,
    const std::vector<size_t>& special_pieces,
    OneSidedType side
) {
    if (ordinary_across_pieces.empty() || special_pieces.empty())
        return {};

    const Face& face = m_adjacencies.get_faces()[face_index];
    const size_t cycle_size = face.path().number_of_nodes() - 1;
    const CylinderNodePositions node_positions =
        compute_cylinder_node_positions(m_adjacencies, face_index);

    const std::vector<size_t>& special_mapping = (side == OneSidedType::DOWN)
                                                     ? node_positions.node_to_down_pos
                                                     : node_positions.node_to_up_pos;

    std::vector<std::vector<size_t>> special_positions(special_pieces.size());
    for (size_t k = 0; k < special_pieces.size(); ++k) {
        const Bridge& bridge = m_adjacencies.get_pieces()[special_pieces[k]];
        special_positions[k] = extract_piece_positions(bridge, special_mapping);
    }

    std::vector<Conflict> conflicts;
    for (size_t j = 0; j < ordinary_across_pieces.size(); ++j) {
        const Bridge& ord_bridge = m_adjacencies.get_pieces()[ordinary_across_pieces[j]];
        const std::vector<size_t> ord_pos =
            extract_piece_positions(ord_bridge, node_positions.node_to_down_pos);
        planarity::CycleConflictDetector detector(cycle_size, ord_pos);

        for (size_t k = 0; k < special_pieces.size(); ++k) {
            if (detector.in_conflict(special_positions[k])) {
                conflicts.push_back(Conflict{ordinary_across_pieces[j], special_pieces[k]});
            }
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

    conflicts.non_across_ordinary_pieces = conflicts_non_across_ordinary_pieces(
        face_index,
        ordinary_non_across_pieces,
        special_pieces
    );

    conflicts.onesided_special_pieces =
        conflicts_onesided_special_pieces(face_index, special_pieces);

    conflicts.across_ordinary_pieces_VS_onesided_down_special_pieces =
        conflicts_across_ordinary_pieces_VS_onesided_special_pieces(
            face_index,
            ordinary_across_pieces,
            special_pieces,
            OneSidedType::DOWN
        );

    conflicts.across_ordinary_pieces_VS_onesided_up_special_pieces =
        conflicts_across_ordinary_pieces_VS_onesided_special_pieces(
            face_index,
            ordinary_across_pieces,
            special_pieces,
            OneSidedType::UP
        );

    m_faces_conflicts[face_index] = std::move(conflicts);
}

struct OrdinaryPieceVariable {
    size_t piece_index;
    size_t face_index;
};

struct OneSidedSpecialPieceVariable {
    size_t piece_index;
    OneSidedType type;
};

using Variable = std::variant<OrdinaryPieceVariable, OneSidedSpecialPieceVariable>;

std::optional<PiecesAssignment>
Conflicts::solve(const std::vector<CylinderType>& cylinders_type_current_guess) const {
    Cnf cnf;
    std::vector<std::vector<size_t>> pieces_to_variables(m_adjacencies.get_pieces().size());
    std::vector<Variable> m_variables;
    m_variables.push_back({});
    size_t found_cylinders = 0;

    // initializing variables and clauses for conflicts
    for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
        const Face& face = m_adjacencies.get_faces()[face_index];
        if (face.type() == FaceType::TYPE_1) {
            for (size_t piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                pieces_to_variables[piece_index].push_back(m_variables.size());
                m_variables.push_back(OrdinaryPieceVariable{piece_index, face_index});
            }
            const auto& conflicts = std::get<SimpleFaceConflicts>(m_faces_conflicts[face_index]);
            for (const Conflict& conflict : conflicts.conflicts) {
                int var_1 = static_cast<int>(pieces_to_variables[conflict.p_1].back());
                int var_2 = static_cast<int>(pieces_to_variables[conflict.p_2].back());
                cnf.add_clause({-var_1, -var_2});
            }
        } else {
            const auto& conflicts = std::get<CylinderConflicts>(m_faces_conflicts[face_index]);
            for (size_t piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                pieces_to_variables[piece_index].push_back(m_variables.size());
                m_variables.push_back(OrdinaryPieceVariable{piece_index, face_index});
            }
            for (size_t piece_index : conflicts.non_across_ordinary_pieces) {
                if (m_adjacencies.get_adjacency(piece_index).adjacent_faces.size() == 1)
                    return std::nullopt;
                cnf.add_clause({-static_cast<int>(pieces_to_variables[piece_index].back())});
            }
            if (cylinders_type_current_guess[found_cylinders] == CylinderType::TWO_SIDED) {
                for (size_t piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                    if (!m_adjacencies.is_ordinary_piece_cutting_cylinder(piece_index, face_index))
                        continue;
                    if (m_adjacencies.get_adjacency(piece_index).adjacent_faces.size() == 1)
                        return std::nullopt;
                    cnf.add_clause({-static_cast<int>(pieces_to_variables[piece_index].back())});
                }
            } else {
                for (size_t piece_index : m_adjacencies.special_pieces_in_face(face_index)) {
                    // convention: [0] is DOWN, [1] is UP
                    size_t down = m_variables.size();
                    pieces_to_variables[piece_index].push_back(down);
                    m_variables.push_back(
                        OneSidedSpecialPieceVariable{piece_index, OneSidedType::DOWN}
                    );
                    size_t up = m_variables.size();
                    pieces_to_variables[piece_index].push_back(up);
                    m_variables.push_back(
                        OneSidedSpecialPieceVariable{piece_index, OneSidedType::UP}
                    );
                    cnf.add_clause({-static_cast<int>(down), -static_cast<int>(up)});
                    cnf.add_clause({static_cast<int>(down), static_cast<int>(up)});
                }
                for (const ConflictOneSided& conflict : conflicts.onesided_special_pieces) {
                    const size_t var_1 = (conflict.p_1_type == OneSidedType::DOWN)
                                             ? pieces_to_variables[conflict.p_1][0]
                                             : pieces_to_variables[conflict.p_1][1];
                    const size_t var_2 = (conflict.p_2_type == OneSidedType::DOWN)
                                             ? pieces_to_variables[conflict.p_2][0]
                                             : pieces_to_variables[conflict.p_2][1];
                    cnf.add_clause({-static_cast<int>(var_1), -static_cast<int>(var_2)});
                }
                for (const auto& [ordinary, special] :
                     conflicts.across_ordinary_pieces_VS_onesided_down_special_pieces) {
                    const size_t ordinary_var = pieces_to_variables[ordinary].back();
                    const size_t special_var = pieces_to_variables[special][0];
                    cnf.add_clause(
                        {-static_cast<int>(ordinary_var), -static_cast<int>(special_var)}
                    );
                }
                for (const auto& [ordinary, special] :
                     conflicts.across_ordinary_pieces_VS_onesided_up_special_pieces) {
                    const size_t ordinary_var = pieces_to_variables[ordinary].back();
                    const size_t special_var = pieces_to_variables[special][1];
                    cnf.add_clause(
                        {-static_cast<int>(ordinary_var), -static_cast<int>(special_var)}
                    );
                }
            }
            found_cylinders++;
        }
    }
    // adding clauses -> at least one face per ordinary piece
    for (size_t piece_index : m_adjacencies.all_ordinary_pieces()) {
        std::vector<int> clause_at_least_one_face;
        for (const size_t var : pieces_to_variables[piece_index])
            clause_at_least_one_face.push_back(static_cast<int>(var));
        cnf.add_clause(clause_at_least_one_face);

        if (pieces_to_variables[piece_index].size() > 1) {
            std::vector<int> clause_at_most_one_face;
            for (const size_t var : pieces_to_variables[piece_index])
                clause_at_most_one_face.push_back(-static_cast<int>(var));
            cnf.add_clause(clause_at_most_one_face);
        }
    }

    SatSolverResult result = sat::solve_2_sat(cnf);
    if (result.result == sat::SatSolverResultType::SAT) {
        std::vector<std::optional<std::variant<size_t, OneSidedType>>> assigned_face_of_piece;
        assigned_face_of_piece.resize(m_adjacencies.get_pieces().size());
        for (const int var_value : result.numbers)
            if (var_value > 0) {
                auto var = m_variables[static_cast<size_t>(var_value)];
                switch (var.index()) {
                case 0:
                    assigned_face_of_piece[std::get<OrdinaryPieceVariable>(var).piece_index] =
                        std::get<OrdinaryPieceVariable>(var).face_index;
                    break;
                case 1:
                    assigned_face_of_piece[std::get<OneSidedSpecialPieceVariable>(var)
                                               .piece_index] =
                        std::get<OneSidedSpecialPieceVariable>(var).type;
                    break;
                }
            }
        PiecesAssignment assignment;
        for (size_t piece_index = 0; piece_index < assigned_face_of_piece.size(); piece_index++) {
            const auto& a = assigned_face_of_piece[piece_index];
            if (!a.has_value()) {
                DOMUS_ASSERT(
                    !m_adjacencies.is_ordinary_piece(piece_index),
                    "Conflicts::solve: should be special piece, in particular adjacent to "
                    "two-sided cylinder, since those are the only one with only one choice at the "
                    "moment, so they dont have corresponding variables"
                );
                assignment.push_back(m_adjacencies.get_adjacency(piece_index).adjacent_faces[0]);
            } else {
                assignment.push_back(*a);
            }
        }
        return assignment;
    }
    return std::nullopt;
}

/*


SatSolverResult m_sat_result;
std::vector<std::optional<size_t>> m_assigned_face_of_piece;





*/

} // namespace domus::torus