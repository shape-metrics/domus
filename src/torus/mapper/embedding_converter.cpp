#include "embedding_converter.hpp"

#include <iostream>
#include <print>
#include <string>

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/planarity/tutte.hpp"
#include "domus/torus/faces.hpp"

namespace domus::torus::mapper {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace domus::drawing;
using color::ColorRGB;

constexpr ColorRGB VERTEX_DEFAULT_COLOR = NAVY_RGB;
constexpr ColorRGB EDGE_DEFAULT_COLOR = GRAY_RGB;

class EquivalentEmbeddingBuilder {
    const Graph& m_old_graph;
    const Embedding& m_old_embedding;
    const Face& m_outer_face;

    Graph& m_graph;
    Embedding& m_embedding;
    Attributes& m_attributes;
    NodesLabels<size_t>& m_node_id_to_old;
    EdgesLabels<size_t>& m_edge_id_to_old;

    NodesContainer m_is_border_new_node;
    NodesContainer m_is_border_old_node;
    NodesLabels<size_t> m_old_inner_node_to_new_node;

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
                m_outer_face.repeated_paths()[0].edge_id_at_position(
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
                m_outer_face.repeated_paths()[0].edge_id_at_position(
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
        const size_t old_prev_node_id
    ) {
        std::println("adding inner edges of {}", old_node_id);
        size_t current_old_edge = old_edge_id;
        size_t old_neighbor_id = old_next_node_id;
        while (true) {
            EdgeIter e = m_old_embedding.prev_in_adjacency_list(
                old_node_id,
                old_neighbor_id,
                current_old_edge
            );
            std::println("{}", old_neighbor_id);
            std::println("{}", e.neighbor_id);

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
            const size_t new_edge_id = m_graph.add_edge(new_node_id, new_inner_node_id);
            m_embedding.add_edge(new_node_id, new_inner_node_id, new_edge_id);
            m_embedding.add_edge(new_inner_node_id, new_node_id, new_edge_id);
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
        std::print("daje ");
        for (size_t i = 0; i < inner_face.number_of_edges(); i++) {
            const size_t new_node_id = inner_face.node_id_at_position(i);
            const size_t old_node_id = m_node_id_to_old.get_label(new_node_id);
            std::print("{} ", old_node_id);
        }
        std::println();
        std::print("sk ");
        for (size_t old_node_id : m_old_graph.get_nodes_ids())
            if (m_is_border_old_node.has_node(old_node_id))
                std::print("{} ", old_node_id);
        std::println();
        for (size_t i = 0; i < inner_face.number_of_edges(); i++) {
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
                old_prev_node_id
            );
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
            builder.add_nodes_adjacent_to_border();
        } else {
            //
        }

        // planarity::compute_nodes_positions(
        //     equivalent_embedding.graph,
        //     equivalent_embedding.attributes
        // );

        return equivalent_embedding;
    }
};

EquivalentEmbedding
build_equivalent_embedding(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
    return EquivalentEmbeddingBuilder::build(graph, embedding, outer_face);
}

} // namespace domus::torus::mapper