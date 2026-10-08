#pragma once

#include <vector>

#include "adjacencies.hpp"

namespace domus::torus {

struct Conflict {
    size_t p_1;
    size_t p_2;
};

enum class OneSidedType { UP, DOWN };

struct ConflictOneSided {
    size_t p_1;
    OneSidedType p_1_type;
    size_t p_2;
    OneSidedType p_2_type;
};

struct SimpleFaceConflicts {
    std::vector<Conflict> conflicts;
};

struct CylinderConflicts {
    std::vector<Conflict> ordinary_pieces;

    std::vector<size_t> non_across_ordinary_pieces;

    std::vector<ConflictOneSided> onesided_special_pieces;

    std::vector<Conflict> across_ordinary_pieces_VS_onesided_down_special_pieces;
    std::vector<Conflict> across_ordinary_pieces_VS_onesided_up_special_pieces;
};

enum class CylinderType { ONE_SIDED, TWO_SIDED };

using PiecesAssignment = std::vector<std::variant<size_t, OneSidedType>>;

class Conflicts {
  private:
    const Adjacencies& m_adjacencies;
    std::vector<std::variant<SimpleFaceConflicts, CylinderConflicts>> m_faces_conflicts;

    std::vector<Conflict> conflicts_between_ordinary_pieces_in_face(size_t face_index);
    std::vector<size_t> conflicts_non_across_ordinary_pieces(
        size_t face_index,
        const std::vector<size_t>& ordinary_non_across_pieces,
        const std::vector<size_t>& special_pieces
    );

    std::vector<ConflictOneSided>
    conflicts_onesided_special_pieces(size_t face_index, const std::vector<size_t>& special_pieces);
    std::vector<Conflict> conflicts_across_ordinary_pieces_VS_onesided_special_pieces(
        size_t face_index,
        const std::vector<size_t>& ordinary_across_pieces,
        const std::vector<size_t>& special_pieces,
        OneSidedType side
    );

    void conflicts_cylinder_face(size_t face_index);

  public:
    Conflicts(const Adjacencies& adjacencies);

    const std::vector<Conflict>& simple_face_conflicts(size_t face_index) const;

    std::optional<PiecesAssignment>
    solve(const std::vector<CylinderType>& cylinders_type_current_guess) const;
};

} // namespace domus::torus