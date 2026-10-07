#include "type_2.hpp"

#include "domus/core/debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"
#include "../utils.hpp"
#include "adjacencies.hpp"
#include "conflicts.hpp"
#include "nodes_positions.hpp"
#include "ordinary_pieces.hpp"
#include "planarized_cylinder.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

class Type2Solver {
    enum class InitializationOutcome { NO_SOLUTION, NOTHING_TO_DO, DONE };

    Embedding& m_embedding;
    const std::vector<Face>& m_faces;

    const std::vector<Bridge>& m_pieces;
    const Adjacencies& m_adjacencies;
    const Conflicts& m_conflicts;
    const NodesPositions m_nodes_positions;

    size_t m_number_of_cylinders = 0;
    std::vector<CylinderType> m_cylinders_type_current_guess;

    std::vector<std::vector<int>> m_piece_face_to_variable;

    std::vector<std::optional<PlanarizedCylinder>> m_cylinder_embeddings;
    std::vector<std::optional<CachedOrdinaryEmbedding>> m_cached_ordinary_piece_embedding;

    Type2Solver(
        Embedding& embedding,
        const std::vector<Face>& faces,
        const std::vector<Bridge>& pieces,
        const Adjacencies& adjacencies,
        const Conflicts& conflicts
    )
        : m_embedding(embedding), m_faces(faces), m_pieces(pieces), m_adjacencies(adjacencies),
          m_conflicts(conflicts), m_nodes_positions(adjacencies) {}

    InitializationOutcome init() {
        if (m_pieces.size() == 0)
            return InitializationOutcome::NOTHING_TO_DO;

        m_cylinder_embeddings.resize(m_faces.size());
        for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
            const Face& face = m_faces[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            m_number_of_cylinders++;
            auto cylinder = PlanarizedCylinder::build(face_index, m_adjacencies);
            if (!cylinder.has_value())
                return InitializationOutcome::NO_SOLUTION;
            m_cylinder_embeddings[face_index] = std::move(cylinder);
        }

        m_cylinders_type_current_guess.resize(m_number_of_cylinders);
        m_cached_ordinary_piece_embedding.resize(m_pieces.size());

        for (size_t piece_index = 0; piece_index < m_pieces.size(); piece_index++) {
            const auto& adjacency = m_adjacencies.get_adjacency(piece_index);
            if (adjacency.type == PieceAdjacency::PieceType::SPECIAL)
                continue;

            size_t face_index =
                m_adjacencies.get_adjacency(piece_index)
                    .adjacent_faces[0]; // any face is good since if it is adjacent to
                                        // any other face then the circular order of the
                                        // attachment would only be flipped in that face
            const Bridge& bridge = m_pieces[piece_index];
            const Face& face = m_faces[face_index];

            m_cached_ordinary_piece_embedding[piece_index] =
                CachedOrdinaryEmbedding::cache_ordinary_embedding(face_index, face, bridge);
            if (!m_cached_ordinary_piece_embedding[piece_index].has_value())
                return InitializationOutcome::NO_SOLUTION;
        }

        return InitializationOutcome::DONE;
    }

    // we have not tested yet if the special pieces can be embedded in a one-sided way inside
    // one-sided cylinders. hence why this function returns a bool: in case the one-sided assigned
    // special pieces cannot be embedded in a one-sided way, the function returns false, otherwise
    // it returns true

    // first, might be useful to check whether that cylinder actually received an ordinary piece
    // which is across in the cylinder, because in case it did not, then there is no point in
    // forcing special pieces to be one-sided in the particular cylinder.

    // then, in case it actually received an across ordinary piece (at least one): we already know
    // that these across pieces are not in conflict with any of the special pieces in the cylinder,
    // since it was tested by the Conflict class. What we might do, at this point, is construct yet
    // another PlanarizedCylinder, putting all the special pieces but this time we also insert the
    // across pieces inside.

    // if this function finally returns true, than the all the onesided cylinders are embedded into
    // the destination embedding of the whole graph, otherwise it returns false and no modification
    // is done on the destination embedding
    bool embed_special_pieces_one_sided_cylinder(const PiecesAssignment& assignment) const {
        struct OneSidedCylinderToEmbed {
            size_t face_index;
            PlanarizedCylinder cylinder;
        };
        std::vector<OneSidedCylinderToEmbed> cylinders_to_embed;

        size_t found_cylinders = 0;
        for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
            const Face& face = m_adjacencies.get_faces()[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            if (m_cylinders_type_current_guess[found_cylinders++] == CylinderType::TWO_SIDED)
                continue;

            const auto& special_pieces = m_adjacencies.special_pieces_in_face(face_index);
            if (special_pieces.empty())
                continue; // No special pieces to embed in this cylinder.

            // This cylinder is ONE_SIDED.
            // Check whether that cylinder actually received an ordinary piece which is across in
            // the cylinder.
            std::vector<size_t> across_pieces;
            for (size_t ord_piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                DOMUS_ASSERT(
                    std::holds_alternative<size_t>(assignment[ord_piece_index]),
                    "Type2Solver::embed_special_pieces_one_sided_cylinder: ordinary piece has "
                    "wrong assignment"
                );
                if (std::get<size_t>(assignment[ord_piece_index]) == face_index &&
                    m_adjacencies.is_ordinary_piece_cutting_cylinder(ord_piece_index, face_index)) {
                    across_pieces.push_back(ord_piece_index);
                }
            }

            if (across_pieces.empty()) {
                // If it did not receive an across ordinary piece, there is no point in forcing
                // special pieces to be one-sided in this particular cylinder, we would get the
                // exact same result in some other computation in which we guessed this cylinder to
                // be two-sided
                return false;
                // so let us procrastinate until that guess happens
                // TODO maybe not the smartest thing? definitely the easiest tho
            } else {
                // In case it actually received at least one across ordinary piece:
                // Construct a PlanarizedCylinder putting all special pieces and across pieces
                // inside.
                auto cylinder = PlanarizedCylinder::build(face_index, m_adjacencies, across_pieces);
                if (!cylinder.has_value())
                    return false;

                cylinders_to_embed.push_back(
                    OneSidedCylinderToEmbed{face_index, std::move(*cylinder)}
                );
            }
        }

        // All one-sided cylinders succeeded. Now embed the special pieces into m_embedding.
        for (const OneSidedCylinderToEmbed& item : cylinders_to_embed)
            item.cylinder.merge_into_embedding(m_adjacencies, m_embedding);

        DOMUS_ASSERT(
            compute_embedding_genus(m_embedding) == 1,
            "Type2Solver::embed_special_pieces_one_sided_cylinder: found embedding should have "
            "genus 1"
        );
        return true;
    }

    // any embeddability without constraints of special pieces in a cylinder is guaranteed to
    // work, since we tested this in advance. hence why this function is void
    void embed_special_pieces_two_sided_cylinder() const {
        size_t found_cylinders = 0;
        for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
            const Face& face = m_adjacencies.get_faces()[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            if (m_cylinders_type_current_guess[found_cylinders++] == CylinderType::ONE_SIDED)
                continue;

            const auto& special_pieces = m_adjacencies.special_pieces_in_face(face_index);
            if (special_pieces.empty())
                continue;

            m_cylinder_embeddings[face_index]->merge_into_embedding(m_adjacencies, m_embedding);
        }
        DOMUS_ASSERT(
            compute_embedding_genus(m_embedding) == 1,
            "Type2Solver::embed_special_pieces_two_sided_cylinder: found embedding should have "
            "genus 1"
        );
    }

    size_t get_incoming_edge_to_node_in_face(const Face& face, size_t node_id) const {
        const auto& path = face.path();
        for (size_t i = 0; i < path.number_of_edges(); ++i) {
            if (path.get_node_id_at_position(i) == node_id) {
                const size_t in_pos = (i == 0) ? (path.number_of_edges() - 1) : (i - 1);
                return path.get_edge_id_at_position(in_pos);
            }
        }
        DOMUS_ASSERT(
            false,
            "Type2Solver::get_incoming_edge_to_node_in_face: node not found in face"
        );
        return 0;
    }

    // any embeddability of ordinary pieces is guaranteed to work, since we tested this in
    // advance. hence why this function is void
    void embed_ordinary_pieces(const PiecesAssignment& assignment) {
        for (size_t ordinary_piece_index : m_adjacencies.all_ordinary_pieces()) {
            CachedOrdinaryEmbedding& cached_embedding =
                *m_cached_ordinary_piece_embedding[ordinary_piece_index];
            const Bridge& bridge = m_pieces[ordinary_piece_index];
            size_t face_index = std::get<size_t>(assignment[ordinary_piece_index]);
            bool same_face =
                (face_index ==
                 cached_embedding.embedded_face_index); // TODO check also if it actually got placed
                                                        // inside the face! might be outside of it!

            if (!same_face)
                cached_embedding.embedding.reverse_all_circular_orders();
            const Embedding& piece_embedding = cached_embedding.embedding;

            // embed internal nodes and their incident edges into m_embedding
            for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
                if (bridge.is_attachment(bridge_node_id))
                    continue;
                const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
                const size_t new_node_id =
                    cached_embedding.node_old_to_new_id.get_label(old_node_id);

                for (const auto edge : piece_embedding.get_edges(new_node_id)) {
                    const size_t old_neighbor_id =
                        cached_embedding.node_new_to_old_id.get_label(edge.neighbor_id);
                    const size_t old_edge_id =
                        cached_embedding.edge_new_to_old_id.get_label(edge.id);
                    m_embedding.add_edge(old_node_id, old_neighbor_id, old_edge_id);
                }
            }

            // embed bridge edges incident to attachments into m_embedding
            // TODO this loop can probably made more efficient by looping on just the attachments
            const Face& assigned_face = m_faces[face_index];
            for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
                const size_t old_edge_id =
                    bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
                const size_t old_from_id =
                    bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id);
                const size_t old_to_id =
                    bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id);

                if (bridge.is_attachment(bridge_edge.edge.from_id)) {
                    const size_t e_in =
                        get_incoming_edge_to_node_in_face(assigned_face, old_from_id);
                    m_embedding.add_edge_after(old_from_id, old_to_id, old_edge_id, e_in);
                }
                if (bridge.is_attachment(bridge_edge.edge.to_id)) {
                    const size_t e_in = get_incoming_edge_to_node_in_face(assigned_face, old_to_id);
                    m_embedding.add_edge_after(old_to_id, old_from_id, old_edge_id, e_in);
                }
            }
        }
        DOMUS_ASSERT(
            compute_embedding_genus(m_embedding) == 1,
            "Type2Solver::embed_ordinary_pieces: found embedding should have genus 1"
        );
    }

    bool solve(size_t done_guesses_of_cylinders) {
        if (done_guesses_of_cylinders == m_number_of_cylinders) {
            DOMUS_ASSERT(
                compute_embedding_genus(m_embedding) == 1,
                "Type2Solver::solve: initial embedding should have genus 1"
            );
            auto piece_to_assigned_face = m_conflicts.solve(m_cylinders_type_current_guess);
            if (!piece_to_assigned_face.has_value())
                return false;
            if (!embed_special_pieces_one_sided_cylinder(*piece_to_assigned_face))
                return false;

            embed_special_pieces_two_sided_cylinder();
            embed_ordinary_pieces(*piece_to_assigned_face);
            return true;
        }
        m_cylinders_type_current_guess[done_guesses_of_cylinders] = CylinderType::ONE_SIDED;
        if (solve(done_guesses_of_cylinders + 1))
            return true;
        m_cylinders_type_current_guess[done_guesses_of_cylinders] = CylinderType::TWO_SIDED;
        if (solve(done_guesses_of_cylinders + 1))
            return true;
        return false;
    }

  public:
    static bool solve_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
        std::vector<Bridge> pieces = Bridge::compute(graph, embedding);
        if (pieces.size() == 0)
            return true;
        const auto adjacencies = Adjacencies::build_adjacencies(pieces, faces, graph);
        if (!adjacencies.has_value()) {
            return false;
        }
        const Conflicts conflicts(*adjacencies);

        Type2Solver solver(embedding, faces, pieces, *adjacencies, conflicts);
        switch (solver.init()) {
        case InitializationOutcome::NO_SOLUTION:
            return false;
        case InitializationOutcome::NOTHING_TO_DO:
            return true;
        case InitializationOutcome::DONE:
            return solver.solve(0);
        }
    }
};

bool handle_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
    add_log_final_configuration(faces);
    return Type2Solver::solve_type_2(graph, embedding, faces);
}

} // namespace domus::torus
