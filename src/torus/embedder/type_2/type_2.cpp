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

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

struct PlanarCylinder {
    Graph graph;
    NodesLabels<size_t> node_new_to_old_id;
    NodesLabels<size_t> node_old_to_new_id;
    EdgesLabels<size_t> edge_new_to_old_id;
    EdgesLabels<size_t> edge_old_to_new_id;
};

struct CachedCylinderEmbedding {
    PlanarCylinder cylinder;
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
            PlanarCylinder cylinder = build_planarized_cylinder(face_index);
            auto result = planarity::compute_planar_embedding(cylinder.graph);
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

    PlanarCylinder build_planarized_cylinder(size_t face_index) {
        PlanarCylinder cylinder;
        const Face& face = m_faces[face_index];
        for (size_t i = 0; i < face.path().number_of_edges(); i++) {
            const size_t prev_node_id = face.path().get_node_id_at_position(i);
            const size_t next_node_id = face.path().get_node_id_at_position(i + 1);
            const size_t edge_id = face.path().get_edge_id_at_position(i);

            if (!cylinder.node_old_to_new_id.has_label(prev_node_id)) {
                const size_t new_node = cylinder.graph.add_node();
                cylinder.node_new_to_old_id.add_label(new_node, prev_node_id);
                cylinder.node_old_to_new_id.add_label(prev_node_id, new_node);
            }
            if (!cylinder.node_old_to_new_id.has_label(next_node_id)) {
                const size_t new_node = cylinder.graph.add_node();
                cylinder.node_new_to_old_id.add_label(new_node, next_node_id);
                cylinder.node_old_to_new_id.add_label(next_node_id, new_node);
            }
            if (!cylinder.edge_old_to_new_id.has_label(edge_id)) {
                const size_t new_prev_node = cylinder.node_old_to_new_id.get_label(prev_node_id);
                const size_t new_next_node = cylinder.node_old_to_new_id.get_label(next_node_id);
                const size_t new_edge_id = cylinder.graph.add_edge(new_prev_node, new_next_node);
                cylinder.edge_new_to_old_id.add_label(new_edge_id, edge_id);
                cylinder.edge_old_to_new_id.add_label(edge_id, new_edge_id);
            }
        }

        // adding incident special pieces to planarized cylinder

        for (size_t piece_index : m_adjacencies.special_pieces_in_face(face_index)) {
            const Bridge& bridge = m_pieces[piece_index];
            for (const size_t bridge_node_id : bridge.get_bridge().get_nodes_ids()) {
                const size_t old_node_id = bridge.get_new_id_to_old_id().get_label(bridge_node_id);
                if (!cylinder.node_old_to_new_id.has_label(old_node_id)) {
                    const size_t new_node_id = cylinder.graph.add_node();
                    cylinder.node_new_to_old_id.add_label(new_node_id, old_node_id);
                    cylinder.node_old_to_new_id.add_label(old_node_id, new_node_id);
                }
            }
            for (const auto bridge_edge : bridge.get_bridge().get_all_edges()) {
                const size_t old_edge_id =
                    bridge.get_new_edge_id_to_old_id().get_label(bridge_edge.id);
                if (!cylinder.edge_old_to_new_id.has_label(old_edge_id)) {
                    const size_t new_prev_id = cylinder.node_old_to_new_id.get_label(
                        bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.from_id)
                    );
                    const size_t new_next_id = cylinder.node_old_to_new_id.get_label(
                        bridge.get_new_id_to_old_id().get_label(bridge_edge.edge.to_id)
                    );
                    const size_t new_edge_id = cylinder.graph.add_edge(new_prev_id, new_next_id);
                    cylinder.edge_new_to_old_id.add_label(new_edge_id, old_edge_id);
                    cylinder.edge_old_to_new_id.add_label(old_edge_id, new_edge_id);
                }
            }
        }

        return cylinder;
    }

    // TODO currently bugged, this approach is too naive
    void adjust_rotation_scheme(
        const Embedding& copy_embedding,
        const PlanarCylinder& cylinder,
        const Face& face,
        Embedding& cylinder_embedding
    ) {
        // rotation scheme in copy_embedding
        const size_t prev_old_node_id = face.repeated_paths()[0].get_first_node_id();
        const size_t next_old_node_id = face.repeated_paths()[0].get_node_id_at_position(1);
        const size_t old_edge_id = face.repeated_paths()[0].get_first_edge_id();

        const auto old_next_edge =
            copy_embedding.next_in_adjacency_list(prev_old_node_id, next_old_node_id, old_edge_id);
        // rotation scheme in planar cylinder
        const size_t prev_new_node_id = cylinder.node_old_to_new_id.get_label(prev_old_node_id);
        const size_t next_new_node_id = cylinder.node_old_to_new_id.get_label(next_old_node_id);
        const size_t new_edge_id = cylinder.edge_old_to_new_id.get_label(old_edge_id);
        const auto new_next_edge = cylinder_embedding.next_in_adjacency_list(
            prev_new_node_id,
            next_new_node_id,
            new_edge_id
        );
        if (old_next_edge.id != cylinder.edge_new_to_old_id.get_label(new_next_edge.id))
            cylinder_embedding.reverse_all_circular_orders();
    }

    std::optional<CachedOrdinaryEmbedding> cache_ordinary_embedding(size_t piece_index) {
        // TODO
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

    void add_special_pieces_to_embedding(
        Embedding& copy_embedding,
        const PlanarCylinder& cylinder,
        const Face& face,
        const Embedding& cylinder_embedding
    ) const {
        for (const size_t new_node_id : cylinder_embedding.get_nodes_ids()) {
            const size_t old_node_id = cylinder.node_new_to_old_id.get_label(new_node_id);
            if (face.is_node_in_repeated_path().get_label(old_node_id).test(0))
                continue;
            for (const auto new_edge : cylinder_embedding.get_edges(new_node_id)) {
                const size_t old_neighbor_id =
                    cylinder.node_new_to_old_id.get_label(new_edge.neighbor_id);
                const size_t old_edge_id = cylinder.edge_new_to_old_id.get_label(new_edge.id);
                // copy_embedding.add_edge(old_node_id, old_neighbor_id, old_edge_id); // only this
                // was here before ai put stuff

                // TODO from here added by ai, nede to check
                if (copy_embedding.has_edge(old_node_id, old_neighbor_id, old_edge_id))
                    continue;
                if (copy_embedding.get_degree_of_node(old_node_id) == 0) {
                    copy_embedding.add_edge(old_node_id, old_neighbor_id, old_edge_id);
                } else {
                    const auto new_prev = cylinder_embedding.prev_in_adjacency_list(
                        new_node_id,
                        new_edge.neighbor_id,
                        new_edge.id
                    );
                    const size_t old_prev_edge_id =
                        cylinder.edge_new_to_old_id.get_label(new_prev.id);
                    copy_embedding.add_edge_after(
                        old_node_id,
                        old_neighbor_id,
                        old_edge_id,
                        old_prev_edge_id
                    );
                }
            }
        }
        for (size_t i = 1; i < face.repeated_paths()[0].number_of_nodes() - 1; i++) {
            const size_t old_node_id = face.repeated_paths()[0].get_node_id_at_position(i);
            // TODO internal nodes of repeated paths
        }
    }

    void embed_special_pieces_two_sided_cylinder() const {
        // Special pieces are verified planar and cached during init().
        // Here we just apply the cached cylinder embeddings to copy_embedding.
        // for (size_t face_index = 0; face_index < m_faces.size(); face_index++) {
        //     const Face& face = m_faces[face_index];
        //     if (face.type() == FaceType::TYPE_1)
        //         continue;
        //     const auto& cached = m_cached_cylinder_embeddings[face_index].value();
        //     add_special_pieces_to_embedding(
        //         copy_embedding,
        //         cached.cylinder,
        //         face,
        //         cached.embedding
        //     );
        // }
        // TODO
    }

    bool embed_ordinary_pieces() {
        // TODO
        return false;
    }

    bool solve(size_t done_guesses_of_cylinders) {
        if (done_guesses_of_cylinders == m_number_of_cylinders) {
            DOMUS_DEBUG_INDENT();
            DOMUS_DEBUG_LN("attempting this guess.");
            auto piece_to_assigned_face = m_conflicts.solve(m_cylinders_type_current_guess);
            if (!piece_to_assigned_face.has_value())
                return false;
            embed_special_pieces_two_sided_cylinder();
            embed_ordinary_pieces();
            DOMUS_DEBUG_LN("embedding extension found.");
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
        if (!adjacencies.has_value())
            return false;
        const Conflicts conflicts(*adjacencies);

        Type2Solver solver(graph, embedding, faces, pieces, *adjacencies, conflicts);
        switch (solver.init()) {
        case InitializationOutcome::NO_SOLUTION:
            DOMUS_DEBUG_LN(
                "no solution: there is a piece that is not attached to any face (is it even "
                "possible?)."
            );
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
            mapper::build_equivalent_embedding(graph, embedding).to_torus_mapping().visualize();
            return solver.solve(0);
        }
    }
};

bool handle_type_2(Graph& graph, Embedding& embedding, const std::vector<Face>& faces) {
    add_log_final_configuration(faces);
    return Type2Solver::solve_type_2(graph, embedding, faces);
}

} // namespace domus::torus
