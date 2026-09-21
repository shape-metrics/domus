#pragma once

#include <optional>
#include <vector>

#include "domus/core/graph/graph_utilities.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"

namespace domus::torus {

struct PieceAdjacency {
    enum class PieceType { ORDINARY, SPECIAL };

    PieceType type;
    std::vector<size_t> adjacent_faces;
};

struct FaceAdjacency {
    std::vector<size_t> ordinary_pieces;
    std::vector<size_t> special_pieces;
};

class Adjacencies {
    struct CylinderBoundaryComponents {
        graph::utilities::NodesContainer component_0;
        graph::utilities::NodesContainer component_1;
    };
    const std::vector<Bridge>& m_pieces;
    const std::vector<Face>& m_faces;
    const graph::Graph& m_graph;
    std::vector<PieceAdjacency> m_pieces_adjacencies;
    std::vector<std::optional<CylinderBoundaryComponents>> m_cylinder_components;

    std::vector<bool> m_is_ordinary;
    std::vector<size_t> m_ordinary_pieces;
    std::vector<bool>
        m_is_across_in_a_cylinder; // is it true that if an ordinary piece is embedded across in a
    // cylinder, it is across in all (at most 2) of them? should be
    std::vector<FaceAdjacency> m_faces_adjacencies;

    const std::vector<std::vector<size_t>> compute_nodes_to_faces();
    bool compute_piece_to_adjacent_faces(const std::vector<std::vector<size_t>>& nodes_to_faces);
    Adjacencies(
        const std::vector<Bridge>& pieces, const std::vector<Face>& faces, const graph::Graph& graph
    );

    void compute_cylinders_boundary_components();

  public:
    static std::optional<Adjacencies> build_adjacencies(
        const std::vector<Bridge>& pieces, const std::vector<Face>& faces, const graph::Graph& graph
    );
    const PieceAdjacency& get_adjacency(size_t piece_index) const;
    const std::vector<Bridge>& get_pieces() const;
    const std::vector<Face>& get_faces() const;
    const graph::Graph& get_graph() const;
    const std::vector<size_t>& special_pieces_in_face(size_t face_index) const;
    const std::vector<size_t>& ordinary_pieces_in_face(size_t face_index) const;
    const std::vector<size_t>& all_ordinary_pieces() const;
    bool is_ordinary_piece(size_t piece_index) const;
    bool is_ordinary_piece_cutting_cylinder(size_t p_index, size_t face_index) const;
};

} // namespace domus::torus