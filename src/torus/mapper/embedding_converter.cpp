#include "embedding_converter.hpp"

#include <string>

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
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

    NodesContainer m_is_border_node;
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
        m_is_border_node.add_node(prev_point);

        for (size_t i = 0; i < m_outer_face.repeated_paths()[0].number_of_edges(); i++) {
            const size_t next_node_id = m_outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const double x = scale.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, 0.0, next_node_id);
            m_is_border_node.add_node(next_point);
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
            const size_t next_node_id = m_outer_face.repeated_paths()[1].node_id_at_position(
                m_outer_face.repeated_paths()[1].number_of_nodes() - i - 2
            );
            const double x = scale.map(
                static_cast<double>(i + m_outer_face.repeated_paths()[0].number_of_nodes())
            );

            const size_t next_point = add_point(x, 0.0, next_node_id);
            m_is_border_node.add_node(next_point);
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
            const size_t next_node_id = m_outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const double x = scale_x_1.map(static_cast<double>(i + 1));
            const double y = scale_y_1.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, y, next_node_id);
            m_is_border_node.add_node(next_point);
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
            const size_t next_node_id = m_outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const double x = scale_x_2.map(static_cast<double>(i + 1));
            const double y = scale_y_2.map(static_cast<double>(i + 1));
            const size_t next_point = add_point(x, y, next_node_id);
            m_is_border_node.add_node(next_point);

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
            const size_t next_node_id = m_outer_face.repeated_paths()[1].node_id_at_position(
                m_outer_face.repeated_paths()[1].number_of_nodes() - i - 2
            );
            const double x = scale.map(static_cast<double>(i + 1)) + last_x;

            const size_t next_point = add_point(x, 1.0, next_node_id);
            m_is_border_node.add_node(next_point);
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
            const size_t next_node_id = m_outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const double x =
                scale.map(
                    static_cast<double>(i + m_outer_face.repeated_paths()[1].number_of_nodes())
                ) +
                last_x;

            size_t next_point;
            if (i == m_outer_face.repeated_paths()[0].number_of_edges() - 1) {
                next_point = corners[2].value();
            } else {
                next_point = add_point(x, 1.0, next_node_id);
                m_is_border_node.add_node(next_point);
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

    void build_inner_nodes() {
        //
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