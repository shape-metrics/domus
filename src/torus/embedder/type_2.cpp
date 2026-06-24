#include "type_2.hpp"

#include <algorithm>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/sat/cnf.hpp"
#include "domus/sat/sat.hpp"

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

enum class InitializationOutcome { NO_SOLUTION, NOTHING_TO_DO, DONE };

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

class Type2Solver {
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

    Type2Solver(Graph& graph, Embedding& embedding, const std::vector<Face>& faces)
        : m_graph(graph), m_embedding(embedding), m_faces(faces) {}

    std::vector<NodesContainer> compute_nodes_in_faces() {
        std::vector<NodesContainer> nodes_in_face;
        for (size_t i = 0; i < m_faces.size(); ++i) {
            const Face& face = m_faces[i];
            nodes_in_face.emplace_back();
            for (size_t j = 0; j < face.path().number_of_nodes() - 1; ++j) {
                const size_t node_id = face.path().node_id_at_position(j);
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

    bool is_ordinary_piece_cutting_cylinder(size_t p_index, size_t face_index) {
        const std::vector<size_t>& attachments = m_old_attachments_of_bridge[p_index];
        const Face& face = m_faces[face_index];
        const Path& path = face.path();
        const auto& is_repeated = face.is_node_in_repeated_path();

        std::vector<std::vector<size_t>> components;
        std::vector<size_t> current_component;

        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t u = path.node_id_at_position(i);
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
            const size_t first_node = path.node_id_at_position(0);
            const size_t last_node = path.node_id_at_position(path.number_of_nodes() - 2);
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
            "Type2Solver::is_ordinary_piece_cutting_cylinder: cylinder face must have exactly two "
            "non-repeated paths"
        );

        bool has_attachment_in_c1 = false;
        for (const size_t u : components[0]) {
            if (std::binary_search(attachments.begin(), attachments.end(), u)) {
                has_attachment_in_c1 = true;
                break;
            }
        }

        bool has_attachment_in_c2 = false;
        for (const size_t u : components[1]) {
            if (std::binary_search(attachments.begin(), attachments.end(), u)) {
                has_attachment_in_c2 = true;
                break;
            }
        }

        return has_attachment_in_c1 && has_attachment_in_c2;
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
        m_special_pieces_in_faces.resize(m_faces.size());
        m_ordinary_pieces_in_faces.resize(m_faces.size());
        m_is_across_in_a_cylinder = std::vector(m_pieces.size(), false);
        m_piece_face_to_variable.resize(m_pieces.size());
        for (size_t i = 0; i < m_pieces.size(); ++i) {
            m_piece_face_to_variable.resize(m_faces.size());
            for (size_t j = 0; j < m_piece_face_to_variable[i].size(); j++)
                m_piece_face_to_variable[i][j] = 0;
        }
        m_variables.push_back({});

        m_pieces = Bridge::compute(m_graph, m_embedding);
        if (m_pieces.size() == 0)
            return InitializationOutcome::NOTHING_TO_DO;
        initialize_old_attachments_bridges();
        std::vector<NodesContainer> nodes_in_faces = compute_nodes_in_faces();

        for (size_t i = 0; i < m_pieces.size(); ++i) {
            std::vector<size_t> adjacent_faces =
                compute_adjacent_faces(m_pieces[i], nodes_in_faces);
            DOMUS_ASSERT(
                adjacent_faces.size() <= 2,
                "handle_type_2: in cubic graphs a piece cannot be adjacent to three or more faces"
            );
            if (adjacent_faces.size() == 0)
                return InitializationOutcome::NO_SOLUTION;
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

        for (size_t i = 0; i < m_faces.size(); i++) {
            if (m_faces[i].type() == FaceType::TYPE_1)
                conflicts_simply_connected_face(i);
            else {
                m_number_of_cylinders++;
                conflicts_cylinder_face(i);
            }
        }

        return InitializationOutcome::DONE;
    }

    bool
    do_attachments_alternate(const std::vector<size_t>& pos_1, const std::vector<size_t>& pos_2) {
        DOMUS_ASSERT(
            pos_1.size() > 1 && pos_2.size() > 1,
            "Type2Solver::do_attachments_alternate: only one attachment"
        );
        for (size_t i = 0; i < pos_1.size(); ++i) {
            const size_t u = pos_1[i];
            const size_t v = pos_1[(i + 1) % pos_1.size()];

            bool all_in_interval = true;
            for (const size_t x : pos_2) {
                bool in_interval = false;
                if (u <= v) {
                    in_interval = (x >= u && x <= v);
                } else {
                    in_interval = (x >= u || x <= v);
                }
                if (!in_interval) {
                    all_in_interval = false;
                    break;
                }
            }
            if (all_in_interval)
                return false;
        }

        return true;
    }

    bool are_ordinary_in_conflict(
        size_t face_index, size_t ordinary_p_1_index, size_t ordinary_p_2_index
    ) {
        const std::vector<size_t>& p_1_attachments =
            m_old_attachments_of_bridge[ordinary_p_1_index];
        const std::vector<size_t>& p_2_attachments =
            m_old_attachments_of_bridge[ordinary_p_2_index];

        std::vector<size_t> pos_1;
        std::vector<size_t> pos_2;

        const Path& path = m_faces[face_index].path();

        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t old_id = path.node_id_at_position(i);
            if (std::binary_search(p_1_attachments.begin(), p_1_attachments.end(), old_id))
                pos_1.push_back(i);
            if (std::binary_search(p_2_attachments.begin(), p_2_attachments.end(), old_id))
                pos_2.push_back(i);
        }

        DOMUS_ASSERT(
            pos_1.size() > 1 && pos_2.size() > 1,
            "Type2Solver::are_ordinary_in_conflict: found only one attachment but we are assuming "
            "biconnectivity"
        );

        return do_attachments_alternate(pos_1, pos_2);
    }

    void conflicts_simply_connected_face(size_t face_index) {
        for (size_t j1 = 0; j1 < m_ordinary_pieces_in_faces[face_index].size() - 1; j1++) {
            const size_t p_1 = m_ordinary_pieces_in_faces[face_index][j1];
            for (size_t j2 = j1 + 1; j2 < m_ordinary_pieces_in_faces[face_index].size(); j2++) {
                const size_t p_2 = m_ordinary_pieces_in_faces[face_index][j2];
                if (are_ordinary_in_conflict(face_index, p_1, p_2))
                    m_conflicts_in_face[face_index].emplace_back(p_1, p_2);
            }
        }
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
            const size_t old_id = path.node_id_at_position(i);

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

        return do_attachments_alternate(pos_1, pos_2);
    }

    void conflicts_cylinder_face(size_t face_index) {
        for (size_t j1 = 0; j1 < m_ordinary_pieces_in_faces[face_index].size() - 1; j1++) {
            const size_t p_1 = m_ordinary_pieces_in_faces[face_index][j1];

            // conflicts between ordinary pieces and special pieces
            for (size_t j2 = 0; j2 < m_special_pieces_in_faces[face_index].size(); j2++) {
                const size_t p_2 = m_special_pieces_in_faces[face_index][j2];
                if (are_ordinary_special_in_conflict(face_index, p_1, p_2)) {
                    // NOTE: if the ordinary piece is in conflict with any of the special pieces,
                    // then the ordinary piece cannot be embedded at all inside the face
                    m_conflicts_in_face[face_index].emplace_back(p_1, p_2);
                }
            }

            // conflicts between ordinary pieces
            for (size_t j2 = j1 + 1; j2 < m_ordinary_pieces_in_faces[face_index].size(); j2++) {
                const size_t p_2 = m_ordinary_pieces_in_faces[face_index][j2];
                if (are_ordinary_in_conflict(face_index, p_1, p_2))
                    m_conflicts_in_face[face_index].emplace_back(p_1, p_2);
            }

            // conflicts between special pieces will be handled later...
        }
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
            const size_t prev_node_id = face.path().node_id_at_position(i);
            const size_t next_node_id = face.path().node_id_at_position(i + 1);
            const size_t edge_id = face.path().edge_id_at_position(i);

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

    void
    add_special_pieces_to_embedding(Embedding& copy_embedding, const PlanarCylinder& cylinder) {
        // TODO
    }

    bool embed_special_pieces(Embedding& copy_embedding) {
        // if a solution is found, however, we did not handle the possible conflicts of special
        // pieces in the cylinders. we take advantage of the fact that each special piece, since the
        // graph is cubic, can only be placed inside one and only one specific cylinder.
        // we then solve the problem by testing planarity of the special pieces inside the cylinder
        // (the cylinder has a trivial planar representation)
        for (size_t face_index = 0; face_index < m_faces.size(); face_index++) {
            const Face& face = m_faces[face_index];
            if (face.type() == FaceType::TYPE_1)
                continue;
            auto cylinder = build_planar_cylinder(face);
            add_special_pieces_to_cylinder(face_index, cylinder);
            auto result = planarity::compute_planar_embedding(cylinder.graph);
            if (!result.has_value())
                return false;
            add_special_pieces_to_embedding(copy_embedding, cylinder);
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
            build_cnf();
            m_sat_result = solve_2_sat(m_cnf);
            if (m_sat_result.result == SatSolverResultType::UNSAT)
                return false;
            m_assigned_face_of_piece = solver_result_to_placement();
            Embedding copy_embedding(m_embedding);
            return (embed_special_pieces(copy_embedding) && embed_ordinary_pieces(copy_embedding));
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
        Type2Solver solver(graph, embedding, faces);
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
