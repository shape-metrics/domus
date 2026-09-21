#include "cnf_builder.hpp"

#include <algorithm>

#include "adjacencies.hpp"
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

struct OrdinaryPiecesConflict {
    size_t p_1;
    size_t p_2;
};

Conflicts::Conflicts(const Adjacencies& adjacencies) : m_adjacencies(adjacencies) {
    for (size_t face_index = 0; face_index < adjacencies.get_faces().size(); face_index++) {
        const Face& face = adjacencies.get_faces()[face_index];
        if (face.type() == FaceType::TYPE_1)
            detect_conflicts_simple_face(face_index);
        else
            detect_conflicts_cylinder_face(face_index);
    }
}

void Conflicts::detect_conflicts_simple_face(size_t face_index) {
    const std::vector<size_t>& pieces = m_adjacencies.ordinary_pieces_in_face(face_index);
    // TODO
}

void Conflicts::detect_conflicts_cylinder_face(size_t face_index) {
    const std::vector<size_t>& ordinary_pieces = m_adjacencies.ordinary_pieces_in_face(face_index);
    const std::vector<size_t>& special_pieces = m_adjacencies.ordinary_pieces_in_face(face_index);
    // TODO
}

} // namespace domus::torus