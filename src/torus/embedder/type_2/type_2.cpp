#include "type_2.hpp"

#include "domus/core/debug.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/torus/embedding_converter.hpp"

#include "../../bridge.hpp"
#include "../../faces.hpp"
#include "../utils.hpp"
#include "adjacencies.hpp"
#include "conflicts.hpp"
#include "planarized_cylinder.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

struct CachedCylinderEmbedding {
    PlanarizedCylinder cylinder;
    Embedding embedding;
};

struct CachedOrdinaryEmbedding {
    Graph ordinary_plus_face;
    Embedding embedding;
    NodesLabels<size_t> node_new_to_old_id;
    NodesLabels<size_t> node_old_to_new_id;
    EdgesLabels<size_t> edge_new_to_old_id;
    EdgesLabels<size_t> edge_old_to_new_id;
    size_t embedded_face_index;
};

class Type2Solver {
    enum class InitializationOutcome { NO_SOLUTION, NOTHING_TO_DO, DONE };

    Graph& m_graph;
    Embedding& m_embedding;
    const std::vector<Face>& m_faces;

    const std::vector<Bridge>& m_pieces;
    const Adjacencies& m_adjacencies;
    const Conflicts& m_conflicts;

    size_t m_number_of_cylinders = 0;
    std::vector<CylinderType> m_cylinders_type_current_guess;

    std::vector<std::vector<int>> m_piece_face_to_variable;

    std::vector<std::optional<CachedCylinderEmbedding>> m_cached_cylinder_embeddings;
    std::vector<std::optional<CachedOrdinaryEmbedding>> m_cached_ordinary_piece_embedding;

    Type2Solver(
        Graph& graph,
        Embedding& embedding,
        const std::vector<Face>& faces,
        const std::vector<Bridge>& pieces,
        const Adjacencies& adjacencies,
        const Conflicts& conflicts
    )
        : m_graph(graph), m_embedding(embedding), m_faces(faces), m_pieces(pieces),
          m_adjacencies(adjacencies), m_conflicts(conflicts) {}

    InitializationOutcome init() {
        if (m_pieces.size() == 0)
            return InitializationOutcome::NOTHING_TO_DO;

        m_cached_cylinder_embeddings.resize(m_faces.size());
        for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
            const Face& face = m_faces[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            m_number_of_cylinders++;
            PlanarizedCylinder cylinder = PlanarizedCylinder::build(face_index, m_adjacencies);
            auto result = planarity::compute_planar_embedding(cylinder.get_graph());
            if (!result.has_value())
                return InitializationOutcome::NO_SOLUTION;
            adjust_rotation_scheme(m_embedding, cylinder, face, result.value());
            m_cached_cylinder_embeddings[face_index] =
                CachedCylinderEmbedding{std::move(cylinder), std::move(result.value())};
        }

        m_cylinders_type_current_guess.resize(m_number_of_cylinders);
        m_cached_ordinary_piece_embedding.resize(m_pieces.size());

        for (size_t piece_index = 0; piece_index < m_pieces.size(); piece_index++) {
            const auto& adjacency = m_adjacencies.get_adjacency(piece_index);
            if (adjacency.type == PieceAdjacency::PieceType::SPECIAL)
                continue;
            m_cached_ordinary_piece_embedding[piece_index] = cache_ordinary_embedding(piece_index);
            if (!m_cached_ordinary_piece_embedding[piece_index].has_value())
                return InitializationOutcome::NO_SOLUTION;
        }

        return InitializationOutcome::DONE;
    }

    // TODO this might be too naive
    // maybe reversing all circular orders under some specific condition is not enough to guarantee
    // consistency. maybe we need to reverse only some nodes' circular order.
    bool is_cylinder_embedding_consistent(
        const PlanarizedCylinder& cylinder,
        const Embedding& cylinder_embedding,
        const Face& face,
        const std::vector<size_t>& pieces_in_cylinder
    ) const {
        for (size_t piece_index : pieces_in_cylinder) {
            const Bridge& bridge = m_pieces[piece_index];
            for (const size_t old_att_id : bridge.get_old_attachments()) {
                if (face.is_node_in_repeated_path().get_label(old_att_id).test(0))
                    continue;

                // old_att_id is on a non-repeated chain, so it appears exactly once in face.path()
                const auto& path = face.path();
                size_t pos = 0;
                for (size_t i = 0; i < path.number_of_edges(); ++i) {
                    if (path.get_node_id_at_position(i) == old_att_id) {
                        pos = i;
                        break;
                    }
                }
                const size_t in_pos = (pos == 0) ? (path.number_of_edges() - 1) : (pos - 1);
                const size_t u = path.get_node_id_at_position(in_pos);
                const size_t e_in = path.get_edge_id_at_position(in_pos);
                const size_t e_out = path.get_edge_id_at_position(pos);

                const size_t new_att_id = cylinder.get_node_old_to_new_id().get_label(old_att_id);
                const size_t new_u = cylinder.get_node_old_to_new_id().get_label(u);
                const size_t new_e_in = cylinder.get_edge_old_to_new_id().get_label(e_in);
                const size_t new_e_out = cylinder.get_edge_old_to_new_id().get_label(e_out);

                const EdgeIter next_edge =
                    cylinder_embedding.next_in_adjacency_list(new_att_id, new_u, new_e_in);
                return (next_edge.id != new_e_out);
            }
        }

        // Fallback: check relative cyclic order of boundary edges at endpoint of repeated path
        if (!face.repeated_paths().empty()) {
            const size_t node_0 = face.repeated_paths()[0].get_first_node_id();
            const size_t node_1 = face.repeated_paths()[0].get_node_id_at_position(1);
            const size_t e1 = face.repeated_paths()[0].get_first_edge_id();

            if (m_embedding.get_degree_of_node(node_0) == 3) {
                const EdgeIter next_iter = m_embedding.next_in_adjacency_list(node_0, node_1, e1);
                const size_t e2 = next_iter.id;
                const size_t e3 =
                    m_embedding.next_in_adjacency_list(node_0, next_iter.neighbor_id, e2).id;

                const size_t new_node_0 = cylinder.get_node_old_to_new_id().get_label(node_0);
                const size_t new_node_1 = cylinder.get_node_old_to_new_id().get_label(node_1);
                const size_t new_e1 = cylinder.get_edge_old_to_new_id().get_label(e1);
                const size_t new_e2 = cylinder.get_edge_old_to_new_id().get_label(e2);
                const size_t new_e3 = cylinder.get_edge_old_to_new_id().get_label(e3);

                EdgeIter curr =
                    cylinder_embedding.next_in_adjacency_list(new_node_0, new_node_1, new_e1);
                while (curr.id != new_e1) {
                    if (curr.id == new_e2)
                        return true;
                    if (curr.id == new_e3)
                        return false;
                    curr = cylinder_embedding
                               .next_in_adjacency_list(new_node_0, curr.neighbor_id, curr.id);
                }
            }
        }

        return true;
    }

    void adjust_rotation_scheme(
        const Embedding& copy_embedding,
        const PlanarizedCylinder& cylinder,
        const Face& face,
        Embedding& cylinder_embedding
    ) const {
        (void)copy_embedding;
        (void)cylinder;
        (void)face;
        (void)cylinder_embedding;
        // TODO
    }

    std::optional<CachedOrdinaryEmbedding> cache_ordinary_embedding(size_t piece_index) {
        size_t face_index = m_adjacencies.get_adjacency(piece_index)
                                .adjacent_faces[0]; // any face is good since if it is adjacent to
                                                    // any other face then the circular order of the
                                                    // attachment would only be flipped in that face

        // we now want to try to embed this piece into this face. we do this by building a graph
        // which is basically the union of the face and the piece and then check if it is planar.
        // if it is not then there is no hope of a future extension.
        CachedOrdinaryEmbedding cached;
        cached.embedded_face_index = face_index;
        const Face& face = m_faces[face_index];

        for (size_t i = 0; i < face.path().number_of_edges(); i++) {
            const size_t prev_node_id = face.path().get_node_id_at_position(i);
            const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
            const size_t edge_id = face.path().get_edge_id_at_position(i);

            if (!cached.node_old_to_new_id.has_label(prev_node_id)) {
                const size_t new_node = cached.ordinary_plus_face.add_node();
                cached.node_new_to_old_id.add_label(new_node, prev_node_id);
                cached.node_old_to_new_id.add_label(prev_node_id, new_node);
            }
            if (!cached.node_old_to_new_id.has_label(next_node_id)) {
                const size_t new_node = cached.ordinary_plus_face.add_node();
                cached.node_new_to_old_id.add_label(new_node, next_node_id);
                cached.node_old_to_new_id.add_label(next_node_id, new_node);
            }
            if (!cached.edge_old_to_new_id.has_label(edge_id)) {
                const size_t new_prev_node = cached.node_old_to_new_id.get_label(prev_node_id);
                const size_t new_next_node = cached.node_old_to_new_id.get_label(next_node_id);
                const size_t new_edge_id =
                    cached.ordinary_plus_face.add_edge(new_prev_node, new_next_node);
                cached.edge_new_to_old_id.add_label(new_edge_id, edge_id);
                cached.edge_old_to_new_id.add_label(edge_id, new_edge_id);
            }
        }

        const Bridge& bridge = m_pieces[piece_index];
        for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
            const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
            if (!cached.node_old_to_new_id.has_label(old_node_id)) {
                const size_t new_node_id = cached.ordinary_plus_face.add_node();
                cached.node_new_to_old_id.add_label(new_node_id, old_node_id);
                cached.node_old_to_new_id.add_label(old_node_id, new_node_id);
            }
        }
        for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
            const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
            if (!cached.edge_old_to_new_id.has_label(old_edge_id)) {
                const size_t new_prev_id = cached.node_old_to_new_id.get_label(
                    bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id)
                );
                const size_t new_next_id = cached.node_old_to_new_id.get_label(
                    bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id)
                );
                const size_t new_edge_id =
                    cached.ordinary_plus_face.add_edge(new_prev_id, new_next_id);
                cached.edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
                cached.edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
            }
        }

        auto result = planarity::compute_planar_embedding(cached.ordinary_plus_face);
        if (!result.has_value())
            return std::nullopt;

        cached.embedding = std::move(result.value());
        return cached;
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

    // if this function finally returns bool, than the initial embedding must NOT be modified from
    // how it initially was. also, note that once we compute the PlanarizedCylinder, even if the
    // procedure succedes, once obtained it might not be ready as it is to be used to emebed pieces
    // inside the real cylinder. that is becasue the rotation scheme of the PlanarizedCylinder might
    // be such that stuff would acually end up being placed outside of it in the real cylinder
    // (stuff with attachments in the non-repeated chains, in particular). before actually using the
    // PlanarCylinder, we must check whether its embedding is consistent with the real cylinder
    // we're working with. if it is not, then we simply have to reverse all circular orders in it,
    // and then it will be good.

    // TODO I don't really know if there is a point, however, in caching the result this time. at
    // the moment we dont cache the embedding produced inside this function.
    bool embed_special_pieces_one_sided_cylinder(const PiecesAssignment& assignment) const {
        struct CylinderToEmbed {
            size_t face_index;
            PlanarizedCylinder cylinder;
            Embedding embedding;
        };
        std::vector<CylinderToEmbed> cylinders_to_embed;

        size_t found_cylinders = 0;
        for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
            const Face& face = m_adjacencies.get_faces()[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            if (m_cylinders_type_current_guess[found_cylinders++] == CylinderType::TWO_SIDED)
                continue;

            // This cylinder is ONE_SIDED.
            // Check whether that cylinder actually received an ordinary piece which is across in
            // the cylinder.
            std::vector<size_t> across_pieces;
            for (size_t ord_piece_index : m_adjacencies.ordinary_pieces_in_face(face_index)) {
                if (std::holds_alternative<size_t>(assignment[ord_piece_index]) &&
                    std::get<size_t>(assignment[ord_piece_index]) == face_index &&
                    m_adjacencies.is_ordinary_piece_cutting_cylinder(ord_piece_index, face_index)) {
                    across_pieces.push_back(ord_piece_index);
                }
            }

            const auto& special_pieces = m_adjacencies.special_pieces_in_face(face_index);
            if (special_pieces.empty()) {
                // No special pieces to embed in this cylinder.
                continue;
            }

            if (across_pieces.empty()) {
                // If it did not receive an across ordinary piece, there is no point in forcing
                // special pieces to be one-sided in this particular cylinder.
                DOMUS_ASSERT(
                    m_cached_cylinder_embeddings[face_index].has_value(),
                    "embed_special_pieces_one_sided_cylinder: cached cylinder embedding missing"
                );
                PlanarizedCylinder cylinder = m_cached_cylinder_embeddings[face_index]->cylinder;
                Embedding embedding = m_cached_cylinder_embeddings[face_index]->embedding;

                if (!is_cylinder_embedding_consistent(cylinder, embedding, face, special_pieces)) {
                    embedding.reverse_all_circular_orders();
                }

                cylinders_to_embed.push_back(
                    CylinderToEmbed{face_index, std::move(cylinder), std::move(embedding)}
                );
            } else {
                // In case it actually received at least one across ordinary piece:
                // Construct a PlanarizedCylinder putting all special pieces and across pieces
                // inside.
                PlanarizedCylinder cylinder = PlanarizedCylinder::build(face_index, m_adjacencies);
                for (size_t across_piece_index : across_pieces) {
                    cylinder.add_piece(m_pieces[across_piece_index]);
                }

                auto result = planarity::compute_planar_embedding(cylinder.get_graph());
                if (!result.has_value()) {
                    return false;
                }

                Embedding embedding = std::move(result.value());

                std::vector<size_t> pieces_in_cylinder = special_pieces;
                pieces_in_cylinder
                    .insert(pieces_in_cylinder.end(), across_pieces.begin(), across_pieces.end());

                if (!is_cylinder_embedding_consistent(
                        cylinder,
                        embedding,
                        face,
                        pieces_in_cylinder
                    )) {
                    embedding.reverse_all_circular_orders();
                }

                cylinders_to_embed.push_back(
                    CylinderToEmbed{face_index, std::move(cylinder), std::move(embedding)}
                );
            }
        }

        // All one-sided cylinders succeeded. Now embed the special pieces into m_embedding.
        for (const CylinderToEmbed& item : cylinders_to_embed)
            item.cylinder.embed_special_pieces_from_cylinder_into_embedding(
                item.embedding,
                m_adjacencies,
                m_embedding
            );

        return true;
    }

    // any embeddability without constraints of special pieces in a cylinder is guaranteed to
    // work, since we tested this in advance. hence why this function is void
    void embed_special_pieces_two_sided_cylinder(const PiecesAssignment& assignment) const {
        size_t found_cylinders = 0;
        for (size_t face_index = 0; face_index < m_adjacencies.get_faces().size(); face_index++) {
            const Face& face = m_adjacencies.get_faces()[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            if (m_cylinders_type_current_guess[found_cylinders++] == CylinderType::ONE_SIDED)
                continue;
            // TODO
        }
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

    bool is_embedding_consistant_with_face(
        const CachedOrdinaryEmbedding& cached_embedding, const Bridge& bridge
    ) const {
        // TODO does this work even if the face is a cylinder? it should??
        const Face& face = m_faces[cached_embedding.embedded_face_index];
        const size_t old_attachment = *bridge.get_old_attachments().begin();
        const auto& path = face.path();

        size_t pos = 0;
        for (size_t i = 0; i < path.number_of_edges(); ++i) {
            if (path.get_node_id_at_position(i) == old_attachment) {
                pos = i;
                break;
            }
        }
        const size_t in_pos = (pos == 0) ? (path.number_of_edges() - 1) : (pos - 1);
        const size_t u = path.get_node_id_at_position(in_pos);
        const size_t e_in = path.get_edge_id_at_position(in_pos);
        const size_t e_out = path.get_edge_id_at_position(pos);

        const size_t new_attachment = cached_embedding.node_old_to_new_id.get_label(old_attachment);
        const size_t new_u = cached_embedding.node_old_to_new_id.get_label(u);
        const size_t new_e_in = cached_embedding.edge_old_to_new_id.get_label(e_in);
        const size_t new_e_out = cached_embedding.edge_old_to_new_id.get_label(e_out);

        const EdgeIter next_edge =
            cached_embedding.embedding.next_in_adjacency_list(new_attachment, new_u, new_e_in);
        return (next_edge.id != new_e_out);
    }

    // any embeddability of ordinary pieces is guaranteed to work, since we tested this in
    // advance. hence why this function is void
    void embed_ordinary_pieces(const PiecesAssignment& assignment) {
        for (size_t ordinary_piece_index : m_adjacencies.all_ordinary_pieces()) {
            const CachedOrdinaryEmbedding& cached_embedding =
                *m_cached_ordinary_piece_embedding[ordinary_piece_index];
            const Bridge& bridge = m_pieces[ordinary_piece_index];
            size_t face_index = std::get<size_t>(assignment[ordinary_piece_index]);
            bool same_face = (face_index == cached_embedding.embedded_face_index);
            // the fact that the face is the same does not guarantee that the cached embedding is
            // consistant with where we need to insert it. at a surface level, if the face is not
            // the same then we might need to reverse all the circular orders of the cached
            // embedded, before we can use. however this can be the case even if the face is
            // actually the same, since the embedder might have actually embedded the piece
            // "outside" of the intended face.
            bool is_consistant_with_face =
                is_embedding_consistant_with_face(cached_embedding, bridge);
            bool rotate_circular_order = (same_face != is_consistant_with_face);

            Embedding piece_embedding = cached_embedding.embedding;
            if (rotate_circular_order)
                piece_embedding.reverse_all_circular_orders();

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
    }

    bool solve(size_t done_guesses_of_cylinders) {
        if (done_guesses_of_cylinders == m_number_of_cylinders) {
            DOMUS_DEBUG_INDENT();
            DOMUS_DEBUG_LN("attempting this guess.");
            auto piece_to_assigned_face = m_conflicts.solve(m_cylinders_type_current_guess);
            if (!piece_to_assigned_face.has_value())
                return false;
            DOMUS_DEBUG_LN("found a potential solution.");
            if (!embed_special_pieces_one_sided_cylinder(*piece_to_assigned_face))
                return false;
            embed_special_pieces_two_sided_cylinder(*piece_to_assigned_face);
            embed_ordinary_pieces(*piece_to_assigned_face);
            DOMUS_DEBUG_LN("embedding extension computed.");
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
        DOMUS_DEBUG_LN("trying to complete the embedding extension.");
        std::vector<Bridge> pieces = Bridge::compute(graph, embedding);
        if (pieces.size() == 0)
            return true;
        const auto adjacencies = Adjacencies::build_adjacencies(pieces, faces, graph);
        if (!adjacencies.has_value()) {
            return false;
        }
        const Conflicts conflicts(*adjacencies);

        Type2Solver solver(graph, embedding, faces, pieces, *adjacencies, conflicts);
        switch (solver.init()) {
        case InitializationOutcome::NO_SOLUTION:
            DOMUS_DEBUG_LN("no solution: there is a piece that cannot be embedded.");
            return false;
        case InitializationOutcome::NOTHING_TO_DO:
            DOMUS_DEBUG_LN("found solution: there are no pieces to embed.");
            return true;
        case InitializationOutcome::DONE:
            DOMUS_DEBUG_LN("continuing to look for an extension.");
            DOMUS_DEBUG_LN("number of cylinders: {}", solver.m_number_of_cylinders);
            DOMUS_DEBUG_EXEC(for (const Face& face : faces) {
                if (face.type() == FaceType::TYPE_2)
                    DOMUS_DEBUG("{}", face.to_string());
            });
            // mapper::build_equivalent_embedding(graph, embedding).to_torus_mapping().visualize();
            return solver.solve(0);
        }
    }
};

bool handle_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
    add_log_final_configuration(faces);
    return Type2Solver::solve_type_2(graph, embedding, faces);
}

} // namespace domus::torus
