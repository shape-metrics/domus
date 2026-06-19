#include "embedding_converter.hpp"

#include <print>
#include <string>

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/drawing/draw_elements.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/planarity/tutte.hpp"
#include "domus/torus/faces.hpp"
#include "domus/torus/mapping.hpp"

namespace domus::torus::mapper {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace domus::drawing;
using color::ColorRGB;

constexpr ColorRGB VERTEX_DEFAULT_COLOR = NAVY_RGB;
constexpr ColorRGB EDGE_DEFAULT_COLOR = GRAY_RGB;
constexpr ColorRGB WHEEL_EDGE_COLOR = ColorRGB{0.9f, 0.9f, 0.9f};

class EquivalentEmbeddingBuilder {
    const Graph& m_old_graph;
    const Embedding& m_old_embedding;
    const Face& m_outer_face;

    Graph& m_graph;
    Embedding& m_embedding;
    Attributes& m_attributes;
    NodesLabels<size_t>& m_node_id_to_old;
    EdgesLabels<size_t>& m_edge_id_to_old;

    std::optional<Path> new_inner_face = std::nullopt;

    NodesContainer m_is_border_new_node;
    NodesContainer m_is_border_old_node;
    NodesLabels<size_t> m_old_inner_node_to_new_node;
    EdgesLabels<size_t> m_old_inner_edge_to_new_edge;

    size_t add_point(double x, double y, size_t old_node_id) {
        m_graph.add_node();
        const size_t new_node_id = m_embedding.add_node();
        m_attributes.set_position(new_node_id, x, y);
        m_attributes.set_node_color(new_node_id, VERTEX_DEFAULT_COLOR);
        m_node_id_to_old.add_label(new_node_id, old_node_id);
        m_attributes.set_node_label(new_node_id, std::to_string(old_node_id));
        return new_node_id;
    }

    size_t add_inner_node(size_t old_node_id) {
        size_t new_node_id = m_graph.add_node();
        m_embedding.add_node();
        m_attributes.set_node_color(new_node_id, VERTEX_DEFAULT_COLOR);
        m_node_id_to_old.add_label(new_node_id, old_node_id);
        m_attributes.set_node_label(new_node_id, std::to_string(old_node_id));
        m_old_inner_node_to_new_node.add_label(old_node_id, new_node_id);
        return new_node_id;
    }

    size_t add_line(ColorRGB color, size_t node_id_1, size_t node_id_2, size_t old_edge_id) {
        const size_t edge_id = m_graph.add_edge(node_id_1, node_id_2);
        m_attributes.set_edge_color(edge_id, color);
        m_edge_id_to_old.add_label(edge_id, old_edge_id);
        return edge_id;
    }

    void build_type_4_border() {
        std::array<std::optional<size_t>, 4> corners;

        const ScaleLinear scale(
            0.0,
            static_cast<double>(
                m_outer_face.repeated_paths()[0].number_of_nodes() +
                m_outer_face.repeated_paths()[1].number_of_nodes() - 2
            ),
            0.0,
            1.0,
            true
        );

        // adding the first repeated path in the bottom of the rectangle
        size_t prev_point =
            add_point(0.0, 0.0, m_outer_face.repeated_paths()[0].get_first_node_id());
        corners[0] = prev_point;
        m_is_border_new_node.add_node(prev_point);
        m_is_border_old_node.add_node(m_outer_face.repeated_paths()[0].get_first_node_id());

        for (size_t i = 0; i < m_outer_face.repeated_paths()[0].number_of_edges(); i++) {
            const size_t next_old_node_id =
                m_outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const double x = scale.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, 0.0, next_old_node_id);
            m_is_border_new_node.add_node(next_point);
            if (!m_is_border_old_node.has_node(next_old_node_id))
                m_is_border_old_node.add_node(next_old_node_id);

            const size_t edge_id = add_line(
                GREEN_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[0].edge_id_at_position(i)
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);

            prev_point = next_point;
        }

        // adding the second repeated path in the bottom of the rectangle
        for (size_t i = 0; i < m_outer_face.repeated_paths()[1].number_of_edges(); i++) {
            const size_t next_old_node_id = m_outer_face.repeated_paths()[1].node_id_at_position(
                m_outer_face.repeated_paths()[1].number_of_nodes() - i - 2
            );
            const double x = scale.map(
                static_cast<double>(i + m_outer_face.repeated_paths()[0].number_of_nodes())
            );

            const size_t next_point = add_point(x, 0.0, next_old_node_id);
            m_is_border_new_node.add_node(next_point);
            if (!m_is_border_old_node.has_node(next_old_node_id))
                m_is_border_old_node.add_node(next_old_node_id);

            const size_t edge_id = add_line(
                RED_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[1].edge_id_at_position(
                    m_outer_face.repeated_paths()[1].number_of_edges() - i - 1
                )
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);

            prev_point = next_point;
            if (i == m_outer_face.repeated_paths()[1].number_of_edges() - 1)
                corners[1] = next_point;
        }

        // adding the third repeated path (left diagonal)
        const double last_x =
            scale.map(static_cast<double>(m_outer_face.repeated_paths()[0].number_of_edges()));
        prev_point = corners[0].value();
        const ScaleLinear scale_x_1(
            0.0,
            static_cast<double>(m_outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            last_x,
            true
        );
        const ScaleLinear scale_y_1(
            0.0,
            static_cast<double>(m_outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            1.0,
            true
        );
        for (size_t i = 0; i < m_outer_face.repeated_paths()[2].number_of_edges(); i++) {
            const size_t next_old_node_id =
                m_outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const double x = scale_x_1.map(static_cast<double>(i + 1));
            const double y = scale_y_1.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, y, next_old_node_id);
            m_is_border_new_node.add_node(next_point);
            if (!m_is_border_old_node.has_node(next_old_node_id))
                m_is_border_old_node.add_node(next_old_node_id);

            const size_t edge_id = add_line(
                BLUE_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[2].edge_id_at_position(i)
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);
            prev_point = next_point;

            if (i == m_outer_face.repeated_paths()[2].number_of_edges() - 1)
                corners[3] = next_point;
        }

        // adding the third repeated path (right diagonal)
        prev_point = corners[1].value();
        const ScaleLinear scale_x_2(
            0.0,
            static_cast<double>(m_outer_face.repeated_paths()[2].number_of_nodes() - 1),
            1.0,
            1.0 + last_x,
            true
        );
        const ScaleLinear scale_y_2(
            0.0,
            static_cast<double>(m_outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            1.0,
            true
        );
        for (size_t i = 0; i < m_outer_face.repeated_paths()[2].number_of_edges(); i++) {
            const size_t next_old_node_id =
                m_outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const double x = scale_x_2.map(static_cast<double>(i + 1));
            const double y = scale_y_2.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, y, next_old_node_id);
            m_is_border_new_node.add_node(next_point);
            if (!m_is_border_old_node.has_node(next_old_node_id))
                m_is_border_old_node.add_node(next_old_node_id);

            const size_t edge_id = add_line(
                BLUE_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[2].edge_id_at_position(i)
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);

            prev_point = next_point;
            if (i == m_outer_face.repeated_paths()[2].number_of_edges() - 1)
                corners[2] = next_point;
        }

        // adding the second repeated path in the top of the rectangle
        prev_point = corners[3].value();
        for (size_t i = 0; i < m_outer_face.repeated_paths()[1].number_of_edges(); i++) {
            const size_t next_old_node_id = m_outer_face.repeated_paths()[1].node_id_at_position(
                m_outer_face.repeated_paths()[1].number_of_nodes() - i - 2
            );
            const double x = scale.map(static_cast<double>(i + 1)) + last_x;

            const size_t next_point = add_point(x, 1.0, next_old_node_id);
            m_is_border_new_node.add_node(next_point);
            if (!m_is_border_old_node.has_node(next_old_node_id))
                m_is_border_old_node.add_node(next_old_node_id);

            const size_t edge_id = add_line(
                RED_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[1].edge_id_at_position(
                    m_outer_face.repeated_paths()[1].number_of_edges() - i - 1
                )
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);
            prev_point = next_point;
        }

        // adding the first repeated path in the top of the rectangle
        for (size_t i = 0; i < m_outer_face.repeated_paths()[0].number_of_edges(); i++) {
            const size_t next_old_node_id =
                m_outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const double x =
                scale.map(
                    static_cast<double>(i + m_outer_face.repeated_paths()[1].number_of_nodes())
                ) +
                last_x;

            size_t next_point;
            if (i == m_outer_face.repeated_paths()[0].number_of_edges() - 1) {
                next_point = corners[2].value();
            } else {
                next_point = add_point(x, 1.0, next_old_node_id);
                m_is_border_new_node.add_node(next_point);
                if (!m_is_border_old_node.has_node(next_old_node_id))
                    m_is_border_old_node.add_node(next_old_node_id);
            }

            const size_t edge_id = add_line(
                GREEN_RGB,
                prev_point,
                next_point,
                m_outer_face.repeated_paths()[0].edge_id_at_position(i)
            );
            m_embedding.add_edge(prev_point, next_point, edge_id);
            m_embedding.add_edge(next_point, prev_point, edge_id);
            prev_point = next_point;
        }
    }

    void add_inner_edges(
        const size_t new_node_id,
        const size_t old_node_id,
        const size_t old_next_node_id,
        const size_t old_edge_id,
        const size_t new_edge_id,
        const size_t old_prev_node_id
    ) {
        size_t current_old_edge = old_edge_id;
        size_t prev_new_edge = new_edge_id;
        size_t old_neighbor_id = old_next_node_id;
        while (true) {
            EdgeIter e = m_old_embedding.prev_in_adjacency_list(
                old_node_id,
                old_neighbor_id,
                current_old_edge
            );

            if (old_prev_node_id == e.neighbor_id)
                break;
            current_old_edge = e.id;
            old_neighbor_id = e.neighbor_id;
            DOMUS_ASSERT(
                [&]() {
                    if (m_is_border_old_node.has_node(old_neighbor_id)) {
                        m_old_embedding.print();
                        m_outer_face.print();
                        std::println("{} {} {}", old_prev_node_id, old_node_id, old_next_node_id);
                        std::println("{}", old_neighbor_id);
                        return false;
                    }
                    return true;
                }(),
                "not supposed to happen for now"
            );
            size_t new_inner_node_id =
                (m_old_inner_node_to_new_node.has_label(old_neighbor_id))
                    ? new_inner_node_id = m_old_inner_node_to_new_node.get_label(old_neighbor_id)
                    : add_inner_node(old_neighbor_id);
            const size_t new_e_id =
                add_line(EDGE_DEFAULT_COLOR, new_node_id, new_inner_node_id, e.id);
            m_old_inner_edge_to_new_edge.add_label(e.id, new_e_id);
            m_embedding.add_edge_before(new_node_id, new_inner_node_id, new_e_id, prev_new_edge);
            prev_new_edge = new_e_id;
            m_embedding.add_edge(new_inner_node_id, new_node_id, new_e_id);
            e = m_old_embedding
                    .prev_in_adjacency_list(old_node_id, old_neighbor_id, current_old_edge);
        }
    }

    void add_nodes_adjacent_to_border() {
        const auto faces = compute_faces_in_embedding(m_graph, m_embedding);
        DOMUS_ASSERT(
            faces.size() == 2,
            "EquivalentEmbeddingBuilder::add_nodes_adjacent_to_border: expected 2 faces"
        );
        const Path& inner_face = [&]() {
            const size_t old_edge_id = m_edge_id_to_old.get_label(faces[0].edge_id_at_position(0));
            const size_t old_next_edge_id =
                m_edge_id_to_old.get_label(faces[0].edge_id_at_position(1));
            for (size_t i = 0; i < m_outer_face.path().number_of_edges(); i++) {
                const size_t ed_id = m_outer_face.path().edge_id_at_position(i);
                if (ed_id != old_edge_id)
                    continue;
                const size_t next_ed_id = m_outer_face.path().edge_id_at_position(
                    (i + 1) % m_outer_face.path().number_of_edges()
                );
                if (next_ed_id == old_next_edge_id)
                    return faces[0];
            }
            return faces[1];
        }();

        for (size_t index = 0; index < inner_face.number_of_edges(); index++) {
            const size_t i = inner_face.number_of_edges() - 1 - index;
            const size_t new_node_id = inner_face.node_id_at_position(i);
            const size_t old_node_id = m_node_id_to_old.get_label(new_node_id);

            const size_t new_next_node_id = inner_face.node_id_at_position(i + 1);
            const size_t old_next_node_id = m_node_id_to_old.get_label(new_next_node_id);

            const size_t new_edge_id = inner_face.edge_id_at_position(i);
            const size_t old_edge_id = m_edge_id_to_old.get_label(new_edge_id);

            const size_t new_prev_node_id =
                (i == 0) ? inner_face.node_id_at_position(inner_face.number_of_edges() - 1)
                         : inner_face.node_id_at_position(i - 1);
            const size_t old_prev_node_id = m_node_id_to_old.get_label(new_prev_node_id);

            add_inner_edges(
                new_node_id,
                old_node_id,
                old_next_node_id,
                old_edge_id,
                new_edge_id,
                old_prev_node_id
            );
        }
        new_inner_face = Path(std::move(inner_face));
        new_inner_face->reverse();
    }

    void complete_all_inner_nodes() {
        // first we add missing edges and nodes to the graph

        // nodes

        for (const size_t old_node_id : m_old_graph.get_nodes_ids()) {
            if (m_is_border_old_node.has_node(old_node_id))
                continue;
            if (m_old_inner_node_to_new_node.has_label(old_node_id))
                continue;
            add_inner_node(old_node_id);
        }

        // edges

        for (const size_t old_node_id : m_old_graph.get_nodes_ids()) {
            if (m_is_border_old_node.has_node(old_node_id))
                continue;
            const size_t new_node_id = m_old_inner_node_to_new_node.get_label(old_node_id);
            for (const auto old_edge : m_old_graph.get_out_edges(old_node_id)) {
                if (m_is_border_old_node.has_node(old_edge.neighbor_id))
                    continue;
                const size_t new_neighbor_id =
                    m_old_inner_node_to_new_node.get_label(old_edge.neighbor_id);
                const size_t new_edge_id =
                    add_line(EDGE_DEFAULT_COLOR, new_node_id, new_neighbor_id, old_edge.id);
                m_old_inner_edge_to_new_edge.add_label(old_edge.id, new_edge_id);
            }
        }

        // then we adjust their rotation scheme in the embedding

        // we need to match the just added nodes's rotation scheme to the one of the
        // old_embedding

        // all the border nodes do not need to be touched

        // however some of the inner nodes already have a portion of their rotation scheme settled
        // up (if they have some border node adjacent), so we need to be careful when handling them

        for (const size_t old_node_id : m_old_graph.get_nodes_ids()) {
            if (m_is_border_old_node.has_node(old_node_id))
                continue;
            const size_t new_node_id = m_old_inner_node_to_new_node.get_label(old_node_id);
            if (m_embedding.get_degree_of_node(new_node_id) == 0) {
                // fresh node, means it cannot be adjacent to a border node
                // we can safely add its neighbors to the embedding since they are all inner
                for (const auto old_edge : m_old_embedding.get_edges(old_node_id)) {
                    const size_t new_neighbor_id =
                        m_old_inner_node_to_new_node.get_label(old_edge.neighbor_id);
                    const size_t new_edge_id = m_old_inner_edge_to_new_edge.get_label(old_edge.id);
                    m_embedding.add_edge(new_node_id, new_neighbor_id, new_edge_id);
                }
            } else {
                // already has some edges (however they are all inner)
                auto e = m_embedding.get_edges(new_node_id).front();
                while (m_embedding.get_degree_of_node(new_node_id) <
                       m_old_embedding.get_degree_of_node(old_node_id)) {
                    const size_t old_edge_id = m_edge_id_to_old.get_label(e.id);
                    const size_t old_neighbor_id = m_node_id_to_old.get_label(e.neighbor_id);
                    auto next_old_e = m_old_embedding.next_in_adjacency_list(
                        old_node_id,
                        old_neighbor_id,
                        old_edge_id
                    );
                    if (!m_is_border_old_node.has_node(next_old_e.neighbor_id)) {
                        const size_t new_next_neighbor_id =
                            m_old_inner_node_to_new_node.get_label(next_old_e.neighbor_id);
                        const size_t new_next_edge_id =
                            m_old_inner_edge_to_new_edge.get_label(next_old_e.id);
                        m_embedding.add_edge_after(
                            new_node_id,
                            new_next_neighbor_id,
                            new_next_edge_id,
                            e.id
                        );
                    }
                    e = m_embedding.next_in_adjacency_list(new_node_id, e.neighbor_id, e.id);
                    continue;
                }
            }
        }
    }

    // every face in m_embedding has to be triangulated, with the exception of the face
    // corresponding to new_inner_face, so we get a triconnected graph and we can use Tutte
    void triangulate_inner_faces() {
        auto faces = compute_faces_in_embedding(m_graph, m_embedding);
        for (const auto& face : faces) {
            // if face is the same as new_inner_face skip (note that corresponding paths may be
            // shifted with one relative to the other, however same faces have same ordering of the
            // same edges in the path)
            if (face.number_of_edges() == new_inner_face->number_of_edges()) {
                bool is_same = false;
                for (size_t i = 0; i < face.number_of_edges(); i++) {
                    if (face.edge_id_at_position(0) == new_inner_face->edge_id_at_position(i)) {
                        bool match = true;
                        for (size_t j = 1; j < face.number_of_edges(); j++) {
                            if (face.edge_id_at_position(j) != new_inner_face->edge_id_at_position(
                                                                   (i + j) % face.number_of_edges()
                                                               )) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            is_same = true;
                            break;
                        }
                    }
                }
                if (is_same)
                    continue;
            }

            // otherwise triangulate

            if (face.number_of_edges() <= 3)
                continue;

            const size_t wheel_node_id = m_graph.add_node();
            m_embedding.add_node();
            m_attributes.hide_node(wheel_node_id);
            for (size_t i = 0; i < face.number_of_edges(); ++i) {
                const size_t new_node_id = face.node_id_at_position(i);
                const size_t new_edge_id = face.edge_id_at_position(i);
                const size_t wheel_edge_id = m_graph.add_edge(wheel_node_id, new_node_id);
                m_attributes.set_edge_color(wheel_edge_id, WHEEL_EDGE_COLOR);
                // m_attributes.hide_edge(wheel_edge_id);
                m_embedding.add_edge(wheel_node_id, new_node_id, wheel_edge_id);
                m_embedding.add_edge_before(new_node_id, wheel_node_id, wheel_edge_id, new_edge_id);
            }
            m_embedding.reverse_circular_order(wheel_node_id);
        }
    }

    void back_to_square() {
        constexpr double x = 1.0;
        std::vector<EdgeId> original_edges;
        for (const auto e : m_graph.get_all_edges())
            original_edges.push_back(e);
        for (const auto e : original_edges) {
            size_t node_id_1 = e.edge.from_id;
            size_t node_id_2 = e.edge.to_id;
            double x_1 = m_attributes.get_position_x(node_id_1);
            double x_2 = m_attributes.get_position_x(node_id_2);
            if (x_1 > x_2) {
                double temp_d = x_1;
                x_1 = x_2;
                x_2 = temp_d;
                size_t temp_st = node_id_1;
                node_id_1 = node_id_2;
                node_id_2 = temp_st;
            }
            const double y_1 = m_attributes.get_position_y(node_id_1);
            const double y_2 = m_attributes.get_position_y(node_id_2);
            if (x_1 < x && x_2 > x) {
                const double y = y_1 + (y_2 - y_1) * (x - x_1) / (x_2 - x_1);

                const size_t new_node_id_1 = m_graph.add_node();
                m_attributes.set_position(new_node_id_1, x, y);
                const size_t e_id_1 = m_graph.add_edge(node_id_1, new_node_id_1);

                const size_t new_node_id_2 = m_graph.add_node();
                const size_t new_node_id_3 = m_graph.add_node();
                m_attributes.set_position(new_node_id_2, 0.0, y);
                m_attributes.set_position(new_node_id_3, x_2 - 1.0, y_2);
                const size_t e_id_2 = m_graph.add_edge(new_node_id_2, new_node_id_3);

                m_attributes.set_edge_color(e_id_1, m_attributes.get_edge_color(e.id));
                m_attributes.set_edge_color(e_id_2, m_attributes.get_edge_color(e.id));

                m_attributes.hide_node(new_node_id_1);
                m_attributes.hide_node(new_node_id_2);
                if (m_node_id_to_old.has_label(node_id_2))
                    m_attributes.set_node_label(
                        new_node_id_3,
                        std::to_string(m_node_id_to_old.get_label(node_id_2))
                    );
                else
                    m_attributes.hide_node(new_node_id_3);

                if (m_attributes.is_edge_hidden(e.id)) {
                    m_attributes.hide_edge(e_id_1);
                    m_attributes.hide_edge(e_id_2);
                }
                m_attributes.hide_edge(e.id);

                if (!m_attributes.is_node_hidden(node_id_2))
                    m_attributes.hide_node(node_id_2);
            } else if (x_1 >= x) {
                const size_t new_node_id_1 = m_graph.add_node();
                const size_t new_node_id_2 = m_graph.add_node();
                m_attributes.set_position(new_node_id_1, x_1 - 1.0, y_1);
                m_attributes.set_position(new_node_id_2, x_2 - 1.0, y_2);
                const size_t e_id = m_graph.add_edge(new_node_id_1, new_node_id_2);

                m_attributes.set_edge_color(e_id, m_attributes.get_edge_color(e.id));

                if (m_node_id_to_old.has_label(node_id_1))
                    m_attributes.set_node_label(
                        new_node_id_1,
                        std::to_string(m_node_id_to_old.get_label(node_id_1))
                    );
                else
                    m_attributes.hide_node(new_node_id_1);

                if (m_node_id_to_old.has_label(node_id_2))
                    m_attributes.set_node_label(
                        new_node_id_2,
                        std::to_string(m_node_id_to_old.get_label(node_id_2))
                    );
                else
                    m_attributes.hide_node(new_node_id_2);

                if (m_attributes.is_edge_hidden(e.id))
                    m_attributes.hide_edge(e_id);

                m_attributes.hide_edge(e.id);

                if (x_1 > 1.0 && !m_attributes.is_node_hidden(node_id_1))
                    m_attributes.hide_node(node_id_1);
                if (x_2 > 1.0 && !m_attributes.is_node_hidden(node_id_2))
                    m_attributes.hide_node(node_id_2);
            }
        }
    }

    EquivalentEmbeddingBuilder(
        const Graph& old_graph,
        const Embedding& old_embedding,
        const Face& outer_face,

        EquivalentEmbedding& equivalent_embedding
    )
        : m_old_graph(old_graph), m_old_embedding(old_embedding), m_outer_face(outer_face),
          m_graph(equivalent_embedding.graph), m_embedding(equivalent_embedding.embedding),
          m_attributes(equivalent_embedding.attributes),
          m_node_id_to_old(equivalent_embedding.node_id_to_old),
          m_edge_id_to_old(equivalent_embedding.edge_id_to_old) {}

  public:
    static EquivalentEmbedding
    build(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
        EquivalentEmbedding equivalent_embedding;

        equivalent_embedding.attributes.add_attribute(Attribute::NODES_POSITION);
        equivalent_embedding.attributes.add_attribute(Attribute::NODES_COLOR);
        equivalent_embedding.attributes.add_attribute(Attribute::HIDDEN_NODES);
        equivalent_embedding.attributes.add_attribute(Attribute::EDGES_COLOR);
        equivalent_embedding.attributes.add_attribute(Attribute::HIDDEN_EDGES);
        equivalent_embedding.attributes.add_attribute(Attribute::NODES_LABELS);

        EquivalentEmbeddingBuilder builder(graph, embedding, outer_face, equivalent_embedding);

        if (outer_face.type() == FaceType::TYPE_4) {
            builder.build_type_4_border();
        } else {
            DOMUS_ASSERT(false, "work in progress");
            // builder.build_type_3_border();
        }

        builder.add_nodes_adjacent_to_border();
        builder.complete_all_inner_nodes();
        builder.triangulate_inner_faces();

        DOMUS_ASSERT(
            compute_embedding_genus(equivalent_embedding.embedding) == 0,
            "EquivalentEmbeddingBuilder::build: embedding is not planar"
        );

        for (const size_t new_node_id : equivalent_embedding.embedding.get_nodes_ids())
            if (equivalent_embedding.embedding.get_degree_of_node(new_node_id) == 0)
                equivalent_embedding.attributes.hide_node(new_node_id);

        planarity::compute_nodes_positions(
            equivalent_embedding.graph,
            equivalent_embedding.attributes
        );

        if (outer_face.type() == FaceType::TYPE_4)
            builder.back_to_square();

        return equivalent_embedding;
    }
};

EquivalentEmbedding
build_equivalent_embedding(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
    return EquivalentEmbeddingBuilder::build(graph, embedding, outer_face);
}

TorusMapping EquivalentEmbedding::to_torus_mapping() const {
    TorusMapping mapping;
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (graph.get_degree_of_node(node_id) == 0)
            continue;
        if (attributes.is_node_hidden(node_id))
            continue;
        Circle2D circle{attributes.get_position(node_id), 15.0, GRAY_RGB};
        mapping.add_circle(
            circle,
            static_cast<size_t>(std::stoi(std::string(attributes.get_node_label(node_id))))
        );
    }
    for (const auto& edge : graph.get_all_edges()) {
        if (attributes.is_edge_hidden(edge.id))
            continue;
        Line2D line{
            attributes.get_position(edge.edge.from_id),
            attributes.get_position(edge.edge.to_id),
            attributes.get_edge_color(edge.id)
        };
        mapping.add_line(line);
    }
    return mapping;
}

} // namespace domus::torus::mapper