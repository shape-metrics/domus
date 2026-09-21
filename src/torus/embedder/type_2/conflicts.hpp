#pragma once

#include <vector>

#include "adjacencies.hpp"

namespace domus::torus {

struct Conflict {
    size_t p_1;
    size_t p_2;
};

struct SimpleFaceConflicts {
    std::vector<Conflict> conflicts;
};

struct CylinderConflicts {
    std::vector<Conflict> ordinary_pieces; // ✔
    std::vector<Conflict> onesided_special_pieces;

    std::vector<Conflict> non_across_ordinary_pieces_VS_special_pieces;

    std::vector<Conflict> across_ordinary_pieces_VS_onesided_1_special_pieces;
    std::vector<Conflict> across_ordinary_pieces_VS_onesided_2_special_pieces;
};

enum class CylinderType { ONE_SIDED, TWO_SIDED };

class Conflicts {
  private:
    const Adjacencies& m_adjacencies;
    std::vector<std::variant<SimpleFaceConflicts, CylinderConflicts>> m_faces_conflicts;

    std::vector<Conflict> conflicts_between_ordinary_pieces_in_face(size_t face_index);
    std::vector<Conflict> conflicts_non_across_ordinary_pieces_VS_special_pieces(
        size_t face_index,
        const std::vector<size_t>& ordinary_non_across_pieces,
        const std::vector<size_t>& special_pieces
    );

    void conflicts_cylinder_face(size_t face_index);

  public:
    Conflicts(const Adjacencies& adjacencies);

    const std::vector<Conflict>& simple_face_conflicts(size_t face_index) const;

    std::optional<const std::vector<size_t>>
    solve(std::vector<CylinderType> cylinders_type_current_guess) const;
};

} // namespace domus::torus