#include "type_2.hpp"

#include "domus/core/debug.hpp"
#include "domus/core/graph/embedding.hpp"

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

    std::vector<CylinderType> m_cylinders_type_current_guess;

    std::vector<std::vector<int>> m_piece_face_to_variable;

    Type2Solver(
        Embedding& embedding,
        const std::vector<Face>& faces,
        const std::vector<Bridge>& pieces,
        const Adjacencies& adjacencies,
        const Conflicts& conflicts
    )
        : m_embedding(embedding), m_faces(faces), m_pieces(pieces), m_adjacencies(adjacencies),
          m_conflicts(conflicts) {

        m_cylinders_type_current_guess.resize(adjacencies.get_number_of_cylinders());
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
        std::vector<PlanarizedCylinder> cylinders_to_embed;

        size_t found_cylinders = 0;
        for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
            const Face& face = m_adjacencies.get_faces()[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            if (m_cylinders_type_current_guess[found_cylinders++] == CylinderType::TWO_SIDED)
                continue;

            const auto& special_pieces_indexes = m_adjacencies.special_pieces_in_face(face_index);
            if (special_pieces_indexes.empty())
                continue; // No special pieces to embed in this cylinder.

            // This cylinder is ONE_SIDED.
            // Check whether that cylinder actually received an ordinary piece which is across in
            // the cylinder.
            std::vector<const Bridge*> across_pieces;
            for (size_t ord_piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                DOMUS_ASSERT(
                    std::holds_alternative<size_t>(assignment[ord_piece_index]),
                    "Type2Solver::embed_special_pieces_one_sided_cylinder: ordinary piece has "
                    "wrong assignment"
                );
                if (std::get<size_t>(assignment[ord_piece_index]) == face_index &&
                    m_adjacencies.is_ordinary_piece_cutting_cylinder(ord_piece_index, face_index)) {
                    across_pieces.push_back(&m_pieces[ord_piece_index]);
                }
            }

            if (across_pieces.empty()) {
                // If it did not receive an across ordinary piece, there is no point in forcing
                // special pieces to be one-sided in this particular cylinder, we would get the
                // exact same result in some other computation in which we guessed this cylinder to
                // be two-sided
                return false;
                // so let us procrastinate until that guess happens
                // TODO maybe not the smartest thing? its definitely the easiest tho
            } else {
                // In case it actually received at least one across ordinary piece:
                // Construct a PlanarizedCylinder putting all special pieces and across pieces
                // inside.
                // TODO probably even just one across piece is enough

                std::vector<const Bridge*> special_pieces;
                for (size_t piece_index : special_pieces_indexes)
                    special_pieces.push_back(&m_pieces[piece_index]);

                auto cylinder = PlanarizedCylinder::build(
                    face_index,
                    m_faces,
                    m_embedding,
                    across_pieces,
                    special_pieces
                );
                if (!cylinder.has_value())
                    return false;
                DOMUS_ASSERT(
                    [&]() {
                        Embedding copy = m_embedding;
                        cylinder->merge_into_embedding(m_adjacencies, copy);
                        return compute_embedding_genus(copy) == 1;
                    }(),
                    "Type2Solver::embed_special_pieces_one_sided_cylinder: merging this one sided "
                    "planarized cylinder into the embedding changes its genus.\n{}\n{}",
                    face.to_string(),
                    cylinder->to_string()
                );

                cylinders_to_embed.push_back(std::move(*cylinder));
            }
        }

        // All one-sided cylinders succeeded. Now embed the special pieces into m_embedding.
        for (const PlanarizedCylinder& cylinder : cylinders_to_embed)
            cylinder.merge_into_embedding(m_adjacencies, m_embedding);

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

            m_adjacencies.get_embedded_cylinder(face_index)
                .merge_into_embedding(m_adjacencies, m_embedding);
        }
        DOMUS_ASSERT(
            compute_embedding_genus(m_embedding) == 1,
            "Type2Solver::embed_special_pieces_two_sided_cylinder: found embedding should have "
            "genus 1"
        );
    }

    // any embeddability of ordinary pieces is guaranteed to work, since we tested this in
    // advance. hence why this function is void
    void embed_ordinary_pieces(const PiecesAssignment& assignment) {
        for (size_t ordinary_piece_index : m_adjacencies.all_ordinary_pieces()) {
            size_t assigned_face_index = std::get<size_t>(assignment[ordinary_piece_index]);

            const CachedOrdinaryEmbedding& cached_embedding =
                m_adjacencies.get_cached_embedding(ordinary_piece_index, assigned_face_index);
            cached_embedding.insert_into_embedding(m_embedding);

            DOMUS_ASSERT(
                compute_embedding_genus(m_embedding) == 1,
                "Type2Solver::embed_ordinary_pieces: found embedding should have genus 1 but it "
                "has {}",
                compute_embedding_genus(m_embedding)
            );
        }
    }

    bool solve(size_t done_guesses_of_cylinders) {
        if (done_guesses_of_cylinders == m_adjacencies.get_number_of_cylinders()) {
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
        const auto adjacencies = Adjacencies::build_adjacencies(pieces, faces, graph, embedding);
        if (!adjacencies.has_value())
            return false;

        const Conflicts conflicts(*adjacencies);

        Type2Solver solver(embedding, faces, pieces, *adjacencies, conflicts);
        return solver.solve(0);
    }
};

bool handle_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
    add_log_final_configuration(faces);
    return Type2Solver::solve_type_2(graph, embedding, faces);
}

} // namespace domus::torus
