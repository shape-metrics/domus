#include "type_2.hpp"

#include <algorithm>

#include "domus/core/debug.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/planarity/interlacement.hpp"
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"
#include "domus/torus/embedding_converter.hpp"

#include "../bridge.hpp"
#include "../faces.hpp"
#include "utils.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace sat::cnf;
using namespace sat;

struct Variable {
    size_t piece_index;
    size_t face_index;
};

enum class CylinderType { ONE_SIDED, TWO_SIDED };

struct PiecesConflict {
    size_t p_1;
    size_t p_2;
};

struct PlanarCylinder {
    Graph graph;
    NodesLabels<size_t> node_new_to_old_id;
    NodesLabels<size_t> node_old_to_new_id;
    EdgesLabels<size_t> edge_new_to_old_id;
    EdgesLabels<size_t> edge_old_to_new_id;
};

struct CylinderBoundaryComponents {
    NodesContainer component_0;
    NodesContainer component_1;
};

struct CachedCylinderEmbedding {
    PlanarCylinder cylinder;
    Embedding embedding;
};

class Type2Solver {
    enum class InitializationOutcome { NO_SOLUTION, NOTHING_TO_DO, DONE };

    Graph& m_graph;
    Embedding& m_embedding;
    const std::vector<Face>& m_faces;

    std::vector<Bridge> m_pieces;
    std::vector<std::vector<size_t>> m_old_attachments_of_bridge;

    std::vector<std::vector<size_t>> m_special_pieces_in_faces;
    std::vector<std::vector<size_t>> m_ordinary_pieces_in_faces;
    std::vector<bool>
        m_is_across_in_a_cylinder; // is it true that if an ordinary piece is embedded across in a
                                   // cylinder, it is across in all (at most 2) of them? should be
    std::vector<std::vector<PiecesConflict>> m_conflicts_in_face;
    size_t m_number_of_cylinders = 0;
    std::vector<CylinderType> m_cylinders_type_current_guess;

    std::vector<Variable> m_variables;
    std::vector<std::vector<int>> m_piece_face_to_variable;

    Cnf m_cnf;
    SatSolverResult m_sat_result;
    std::vector<std::optional<size_t>> m_assigned_face_of_piece;

    std::vector<std::optional<CylinderBoundaryComponents>> m_cylinder_components;
    std::vector<std::optional<CachedCylinderEmbedding>> m_cached_cylinder_embeddings;

    Type2Solver(Graph& graph, Embedding& embedding, const std::vector<Face>& faces)
        : m_graph(graph), m_embedding(embedding), m_faces(faces) {}

    std::vector<NodesContainer> compute_nodes_in_faces() {
        std::vector<NodesContainer> nodes_in_face;
        for (size_t i = 0; i < m_faces.size(); ++i) {
            const Face& face = m_faces[i];
            nodes_in_face.emplace_back();
            for (size_t j = 0; j < face.path().number_of_nodes() - 1; ++j) {
                const size_t node_id = face.path().get_node_id_at_position(j);
                if (!nodes_in_face[i].has_node(node_id))
                    nodes_in_face[i].add_node(node_id);
            }
        }
        return nodes_in_face;
    }

    bool is_piece_in_face(const NodesContainer& nodes_in_face, const Bridge& bridge) {
        for (const size_t attachment_id : bridge.get_attachments()) {
            const size_t old_attachment_id = bridge.get_new_id_to_old_id().get_label(attachment_id);
            if (!nodes_in_face.has_node(old_attachment_id))
                return false;
        }
        return true;
    }

    std::vector<size_t> compute_adjacent_faces(
        const Bridge& bridge, const std::vector<NodesContainer>& nodes_in_faces
    ) {
        std::vector<size_t> adjacent_faces;
        for (size_t face_index = 0; face_index < m_faces.size(); face_index++)
            if (is_piece_in_face(nodes_in_faces[face_index], bridge))
                adjacent_faces.push_back(face_index);
        DOMUS_ASSERT(
            adjacent_faces.size() > 0,
            "Type2Solver::compute_adjacent_faces: piece need to be adjacent to at least one face\n"
        );
        return adjacent_faces;
    }

    void initialize_old_attachments_bridges() {
        m_old_attachments_of_bridge.resize(m_pieces.size());
        for (size_t i = 0; i < m_pieces.size(); ++i) {
            const Bridge& p = m_pieces[i];
            m_old_attachments_of_bridge[i].reserve(p.number_of_attachments());
            for (const size_t new_id : p.get_attachments())
                m_old_attachments_of_bridge[i].push_back(
                    p.get_new_id_to_old_id().get_label(new_id)
                );
            std::sort(m_old_attachments_of_bridge[i].begin(), m_old_attachments_of_bridge[i].end());
        }
    }

    CylinderBoundaryComponents compute_cylinder_boundary_components(const Face& face) const {
        const Path& path = face.path();
        const auto& is_repeated = face.is_node_in_repeated_path();

        std::vector<std::vector<size_t>> components;
        std::vector<size_t> current_component;

        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t u = path.get_node_id_at_position(i);
            if (!is_repeated.get_label(u).test(0)) {
                current_component.push_back(u);
            } else {
                if (!current_component.empty()) {
                    components.push_back(current_component);
                    current_component.clear();
                }
            }
        }
        if (!current_component.empty()) {
            components.push_back(current_component);
        }

        if (components.size() > 1) {
            const size_t first_node = path.get_node_id_at_position(0);
            const size_t last_node = path.get_node_id_at_position(path.number_of_nodes() - 2);
            if (!is_repeated.get_label(first_node).test(0) &&
                !is_repeated.get_label(last_node).test(0)) {
                components[0].insert(
                    components[0].end(),
                    components.back().begin(),
                    components.back().end()
                );
                components.pop_back();
            }
        }

        DOMUS_ASSERT(
            components.size() == 2,
            "Type2Solver::compute_cylinder_boundary_components: cylinder face must have exactly "
            "two "
            "non-repeated paths"
        );

        CylinderBoundaryComponents result;
        for (const size_t u : components[0])
            result.component_0.add_node(u);
        for (const size_t u : components[1])
            result.component_1.add_node(u);

        return result;
    }

    bool is_ordinary_piece_cutting_cylinder(size_t p_index, size_t face_index) const {
        const auto& comp = m_cylinder_components[face_index].value();
        bool has_attachment_in_c1 = false;
        bool has_attachment_in_c2 = false;
        for (const size_t u : m_old_attachments_of_bridge[p_index]) {
            if (comp.component_0.has_node(u))
                has_attachment_in_c1 = true;
            if (comp.component_1.has_node(u))
                has_attachment_in_c2 = true;
            if (has_attachment_in_c1 && has_attachment_in_c2)
                return true;
        }
        return false;
    }

    bool is_a_special_piece_in_cylinder(size_t piece_index, size_t face_index) {
        const Bridge& piece = m_pieces[piece_index];
        const Face& face = m_faces[face_index];
        for (const size_t attachment : piece.get_attachments()) {
            const size_t old_attachment = piece.get_new_id_to_old_id().get_label(attachment);
            if (face.is_node_in_repeated_path().get_label(old_attachment).test(0))
                return true;
        }
        return false;
    }

    void initialize_variables(size_t piece_index, const std::vector<size_t>& adjacent_faces) {
        for (size_t i = 0; i < adjacent_faces.size(); i++) {
            const size_t face_index = adjacent_faces[i];
            m_variables.push_back(Variable{piece_index, face_index});
            m_piece_face_to_variable[piece_index][face_index] =
                static_cast<int>(m_variables.size()) - 1;
        }
    }

    InitializationOutcome init() {
        m_pieces = Bridge::compute(m_graph, m_embedding);
        if (m_pieces.size() == 0)
            return InitializationOutcome::NOTHING_TO_DO;

        m_special_pieces_in_faces.resize(m_faces.size());
        m_ordinary_pieces_in_faces.resize(m_faces.size());
        m_conflicts_in_face.resize(m_faces.size());
        m_is_across_in_a_cylinder = std::vector(m_pieces.size(), false);
        m_piece_face_to_variable.assign(m_pieces.size(), std::vector<int>(m_faces.size(), 0));
        m_variables.push_back({});

        initialize_old_attachments_bridges();
        std::vector<NodesContainer> nodes_in_faces = compute_nodes_in_faces();

        m_cylinder_components.resize(m_faces.size());
        for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
            if (m_faces[face_index].type() == FaceType::TYPE_2)
                m_cylinder_components[face_index] =
                    compute_cylinder_boundary_components(m_faces[face_index]);
        }

        for (size_t i = 0; i < m_pieces.size(); ++i) {
            std::vector<size_t> adjacent_faces =
                compute_adjacent_faces(m_pieces[i], nodes_in_faces);
            DOMUS_ASSERT(
                adjacent_faces.size() <= 2,
                "Type2Solver::init: in cubic graphs a piece cannot be adjacent to three or more "
                "faces"
            );
            initialize_variables(i, adjacent_faces);
            for (const size_t face_index : adjacent_faces) {
                if (m_faces[face_index].type() == FaceType::TYPE_2) {
                    if (is_a_special_piece_in_cylinder(i, face_index))
                        m_special_pieces_in_faces[face_index].emplace_back(i);
                    else {
                        m_ordinary_pieces_in_faces[face_index].emplace_back(i);
                        if (!m_is_across_in_a_cylinder[i])
                            m_is_across_in_a_cylinder[i] =
                                is_ordinary_piece_cutting_cylinder(i, face_index);
                    }
                } else {
                    m_ordinary_pieces_in_faces[face_index].emplace_back(i);
                }
            }
        }

        m_cached_cylinder_embeddings.resize(m_faces.size());
        for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
            const Face& face = m_faces[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            PlanarCylinder cylinder = build_planar_cylinder(face);
            add_special_pieces_to_cylinder(face_index, cylinder);
            auto result = planarity::compute_planar_embedding(cylinder.graph);
            if (!result.has_value())
                return InitializationOutcome::NO_SOLUTION;
            adjust_rotation_scheme(m_embedding, cylinder, face, result.value());
            m_cached_cylinder_embeddings[face_index] =
                CachedCylinderEmbedding{std::move(cylinder), std::move(result.value())};
        }

        for (size_t i = 0; i < m_faces.size(); i++) {
            if (m_faces[i].type() == FaceType::TYPE_1)
                conflicts_simply_connected_face(i);
            else {
                m_number_of_cylinders++;
                conflicts_cylinder_face(i);
            }
        }

        m_cylinders_type_current_guess.resize(m_number_of_cylinders);

        return InitializationOutcome::DONE;
    }

    std::vector<size_t>
    get_ordinary_piece_face_positions(size_t face_index, size_t piece_index) const {
        const std::vector<size_t>& p_attachments = m_old_attachments_of_bridge[piece_index];
        std::vector<size_t> pos;
        const Path& path = m_faces[face_index].path();
        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t old_id = path.get_node_id_at_position(i);
            if (std::binary_search(p_attachments.begin(), p_attachments.end(), old_id))
                pos.push_back(i);
        }
        return pos;
    }

    void
    compute_ordinary_conflicts_in_face(size_t face_index, const std::vector<size_t>& ordinary) {
        if (ordinary.size() < 2)
            return;

        const size_t cycle_size = m_faces[face_index].path().number_of_nodes() - 1;
        std::vector<std::vector<size_t>> piece_positions(ordinary.size());
        for (size_t j = 0; j < ordinary.size(); ++j) {
            piece_positions[j] = get_ordinary_piece_face_positions(face_index, ordinary[j]);
            DOMUS_ASSERT(
                piece_positions[j].size() > 1,
                "Type2Solver::compute_ordinary_conflicts_in_face: found only one attachment but we "
                "are assuming biconnectivity"
            );
        }

        for (size_t j1 = 0; j1 < ordinary.size() - 1; ++j1) {
            const size_t p_1 = ordinary[j1];
            planarity::CycleConflictDetector detector(cycle_size, piece_positions[j1]);
            for (size_t j2 = j1 + 1; j2 < ordinary.size(); ++j2) {
                const size_t p_2 = ordinary[j2];
                if (detector.in_conflict(piece_positions[j2]))
                    m_conflicts_in_face[face_index].emplace_back(p_1, p_2);
            }
        }
    }

    bool are_ordinary_in_conflict(
        size_t face_index, size_t ordinary_p_1_index, size_t ordinary_p_2_index
    ) const {
        const std::vector<size_t> pos_1 =
            get_ordinary_piece_face_positions(face_index, ordinary_p_1_index);
        const std::vector<size_t> pos_2 =
            get_ordinary_piece_face_positions(face_index, ordinary_p_2_index);

        DOMUS_ASSERT(
            pos_1.size() > 1 && pos_2.size() > 1,
            "Type2Solver::are_ordinary_in_conflict: found only one attachment but we are assuming "
            "biconnectivity"
        );

        return planarity::are_attachments_in_conflict(
            m_faces[face_index].path().number_of_nodes() - 1,
            pos_1,
            pos_2
        );
    }

    void conflicts_simply_connected_face(size_t face_index) {
        compute_ordinary_conflicts_in_face(face_index, m_ordinary_pieces_in_faces[face_index]);
    }

    bool are_ordinary_special_in_conflict(
        size_t face_index, size_t ordinary_p_index, size_t special_p_index
    ) {
        const std::vector<size_t>& p_1_attachments = m_old_attachments_of_bridge[ordinary_p_index];
        const std::vector<size_t>& p_2_attachments = m_old_attachments_of_bridge[special_p_index];

        std::vector<size_t> pos_1;
        std::vector<size_t> pos_2;

        const Face& face = m_faces[face_index];
        const Path& path = face.path();
        const auto& is_repeated = face.is_node_in_repeated_path();

        bool has_placed_special_attachment = false;

        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t old_id = path.get_node_id_at_position(i);

            if (std::binary_search(p_1_attachments.begin(), p_1_attachments.end(), old_id))
                pos_1.push_back(i);

            if (std::binary_search(p_2_attachments.begin(), p_2_attachments.end(), old_id)) {
                if (is_repeated.get_label(old_id).test(0)) {
                    if (!has_placed_special_attachment) {
                        pos_2.push_back(i);
                        has_placed_special_attachment = true;
                    }
                } else {
                    pos_2.push_back(i);
                }
            }
        }

        DOMUS_ASSERT(
            pos_1.size() > 1 && pos_2.size() > 1,
            "Type2Solver::are_ordinary_special_in_conflict: only one attachment"
        );

        return planarity::are_attachments_in_conflict(path.number_of_nodes() - 1, pos_1, pos_2);
    }

    // TODO TOO NAIVE! if the cylinder is two sided then, since there is no ordinary piece embedded
    // across, every conflict ordinary-special is captured by any embedding of the special piece.
    // but if the cylinder is guessed onesided then there can be ordinary acrosses pieces, however
    // in this case a conflict between this possibly across piece DEPENDS on the choice of the
    // special piece (which has at most 2 choices). this means that if the current guess of the
    // cylinder is one-sided, we need to compute conflicts more carefully
    void conflicts_cylinder_face(size_t face_index) {
        const auto& ordinary = m_ordinary_pieces_in_faces[face_index];
        const auto& special = m_special_pieces_in_faces[face_index];

        // conflicts between ordinary pieces and special pieces
        for (const size_t s : special)
            for (const size_t o : ordinary) {
                if (are_ordinary_special_in_conflict(face_index, o, s)) {
                    // NOTE: if the ordinary piece is in conflict with any of the special pieces,
                    // then the ordinary piece cannot be embedded at all inside the face
                    m_conflicts_in_face[face_index].emplace_back(o, s);
                }
            }

        // conflicts between ordinary pieces
        compute_ordinary_conflicts_in_face(face_index, ordinary);

        // conflicts between special pieces will be handled later...
    }

    void build_cnf() {
        m_cnf = Cnf{};
        size_t found_cylinders = 0;

        // adding clauses which represent conflicts between pieces
        for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
            for (auto [p_1, p_2] : m_conflicts_in_face[face_index]) {
                m_cnf.add_clause(
                    {-m_piece_face_to_variable[p_1][face_index],
                     -m_piece_face_to_variable[p_2][face_index]}
                );
            }
            if (m_faces[face_index].type() == FaceType::TYPE_1)
                continue;

            if (m_cylinders_type_current_guess[found_cylinders++] ==
                CylinderType::TWO_SIDED) // ordinary across pieces in two sided cylinders are
                                         // forbidden
                for (size_t piece_index : m_ordinary_pieces_in_faces[face_index])
                    if (m_is_across_in_a_cylinder[piece_index])
                        m_cnf.add_clause({-m_piece_face_to_variable[piece_index][face_index]});
        }

        // adding clauses to guarantee that each piece is in at least once face
        for (size_t piece_index = 0; piece_index < m_piece_face_to_variable.size(); piece_index++) {
            std::vector<int> in_at_least_one_face;
            for (size_t face_index = 0; face_index < m_piece_face_to_variable[piece_index].size();
                 face_index++) {
                if (m_piece_face_to_variable[piece_index][face_index] == 0)
                    continue;
                in_at_least_one_face.push_back(m_piece_face_to_variable[piece_index][face_index]);
            }
            m_cnf.add_clause(in_at_least_one_face);
        }
    }

    PlanarCylinder build_planar_cylinder(const Face& face) {
        PlanarCylinder cylinder;
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
        return cylinder;
    }

    void add_special_pieces_to_cylinder(size_t face_index, PlanarCylinder& cylinder) {
        for (size_t piece_index : m_special_pieces_in_faces[face_index]) {
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

    bool embed_special_pieces(Embedding& copy_embedding) const {
        // Special pieces are verified planar and cached during init().
        // Here we just apply the cached cylinder embeddings to copy_embedding.
        for (size_t face_index = 0; face_index < m_faces.size(); face_index++) {
            const Face& face = m_faces[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            const auto& cached = m_cached_cylinder_embeddings[face_index].value();
            add_special_pieces_to_embedding(
                copy_embedding,
                cached.cylinder,
                face,
                cached.embedding
            );
        }
        return true;
    }

    bool embed_ordinary_pieces(Embedding& copy_embedding) {
        // TODO
        return false;
    }

    auto solver_result_to_placement() {
        std::vector<std::optional<size_t>> assigned_face_of_piece;
        assigned_face_of_piece.resize(m_pieces.size());
        for (const int var_value : m_sat_result.numbers)
            if (var_value > 0) {
                auto var = m_variables[static_cast<size_t>(var_value)];
                assigned_face_of_piece[var.piece_index] = var.face_index;
            }
        return assigned_face_of_piece;
    }

    bool solve(size_t done_guesses_of_cylinders) {
        if (done_guesses_of_cylinders == m_number_of_cylinders) {
            DOMUS_DEBUG_INDENT();
            DOMUS_DEBUG_LN("attempting this guess.");
            build_cnf();
            DOMUS_DEBUG_LN("cnf built.");
            m_sat_result = solve_2_sat(m_cnf);
            DOMUS_DEBUG_LN("{}", satSolverResultType_to_string(m_sat_result.result));
            if (m_sat_result.result == SatSolverResultType::UNSAT)
                return false;
            m_assigned_face_of_piece = solver_result_to_placement();
            Embedding copy_embedding(m_embedding);
            const bool embedded_special_pieces = embed_special_pieces(copy_embedding);
            const bool embedded_ordinary_pieces = embed_ordinary_pieces(copy_embedding);
            if (!embedded_special_pieces || !embedded_ordinary_pieces) {
                DOMUS_DEBUG_LN("could not extend embedding.");
                if (!embedded_special_pieces)
                    DOMUS_DEBUG_LN("failed to embed special pieces.");
                if (!embedded_ordinary_pieces)
                    DOMUS_DEBUG_LN("failed to embed ordinary pieces.");
                return false;
            }
            m_embedding = std::move(copy_embedding);
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
        Type2Solver solver(graph, embedding, faces);
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
