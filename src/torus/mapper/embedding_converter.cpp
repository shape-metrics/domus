#include "embedding_converter.hpp"

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/drawing/linear_scale.hpp"
#include "domus/torus/mapping.hpp"

namespace domus::torus::mapper {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace domus::drawing;
using color::ColorRGB;

constexpr ColorRGB CIRCLE_DEFAULT_COLOR = NAVY_RGB;
constexpr ColorRGB EDGE_DEFAULT_COLOR = GRAY_RGB;
constexpr float CIRCLE_RADIUS = 15.0f;

class EquivalentEmbeddingBuilder {
    const Graph& old_graph;
    const Embedding& old_embedding;
    const Face& outer_face;

    Graph& graph;
    Embedding& embedding;
    Attributes& attributes;
    NodesLabels<size_t>& node_id_to_old;
    EdgesLabels<size_t>& edge_id_to_old;

    size_t add_point(double x, double y, size_t old_node_id) {
        graph.add_node();
        const size_t new_node_id = embedding.add_node();
        attributes.set_position(new_node_id, x, y);
        attributes.set_node_color(new_node_id, CIRCLE_DEFAULT_COLOR);
        node_id_to_old.add_label(new_node_id, old_node_id);
        return new_node_id;
    }

    size_t add_line(ColorRGB color, size_t node_id_1, size_t node_id_2, size_t old_edge_id) {
        const size_t edge_id = graph.add_edge(node_id_1, node_id_2);
        attributes.set_edge_color(edge_id, color);
        edge_id_to_old.add_label(edge_id, old_edge_id);
        return edge_id;
    }

    void build_border() {
        std::array<std::optional<size_t>, 4> corners;

        const ScaleLinear scale(
            0.0,
            static_cast<double>(
                outer_face.repeated_paths()[0].number_of_nodes() +
                outer_face.repeated_paths()[1].number_of_nodes() - 2
            ),
            0.0,
            1.0,
            true
        );

        // adding the first repeated path in the bottom of the rectangle
        size_t prev_point = add_point(0.0, 0.0, outer_face.repeated_paths()[0].get_first_node_id());
        corners[0] = prev_point;

        for (size_t i = 0; i < outer_face.repeated_paths()[0].number_of_edges(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const size_t edge_id = outer_face.repeated_paths()[0].edge_id_at_position(i);
            const double x = scale.map(static_cast<double>(i + 1));
            const size_t next_point_index = add_point(x, 0.0, next_node_id);
            add_line(GREEN_RGB, prev_point, next_point_index, edge_id);
            prev_point = next_point_index;
        }
        // adding the second repeated path in the bottom of the rectangle
        for (size_t i = 0; i < outer_face.repeated_paths()[1].number_of_edges(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[1].node_id_at_position(
                outer_face.repeated_paths()[1].number_of_nodes() - i - 2
            );
            const size_t edge_id = outer_face.repeated_paths()[0].edge_id_at_position(
                outer_face.repeated_paths()[1].number_of_edges() - i - 1
            );
            const double x = scale.map(
                static_cast<double>(i + outer_face.repeated_paths()[0].number_of_nodes())
            );

            const size_t next_point_index = add_point(x, 0.0, next_node_id);
            add_line(RED_RGB, prev_point, next_point_index, edge_id);
            if (i == outer_face.repeated_paths()[1].number_of_edges() - 1)
                corners[1] = next_point_index;
        }

        // adding the third repeated path (left diagonal)
        const double last_x =
            scale.map(static_cast<double>(outer_face.repeated_paths()[0].number_of_edges()));
        prev_point = corners[0].value();
        const ScaleLinear scale_x_1(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            last_x,
            true
        );
        const ScaleLinear scale_y_1(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            1.0,
            true
        );
        for (size_t i = 0; i < outer_face.repeated_paths()[2].number_of_edges(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const size_t edge_id = outer_face.repeated_paths()[2].edge_id_at_position(i);
            const double x = scale_x_1.map(static_cast<double>(i + 1));
            const double y = scale_y_1.map(static_cast<double>(i + 1));
            const size_t next_point_index = add_point(x, y, next_node_id);
            add_line(BLUE_RGB, prev_point, next_point_index, edge_id);
            prev_point = next_point_index;

            if (i == outer_face.repeated_paths()[2].number_of_edges() - 1)
                corners[3] = next_point_index;
        }

        // adding the third repeated path (right diagonal)
        prev_point = corners[1].value();
        const ScaleLinear scale_x_2(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            1.0,
            1.0 + last_x,
            true
        );
        const ScaleLinear scale_y_2(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            1.0,
            true
        );
        for (size_t i = 0; i < outer_face.repeated_paths()[2].number_of_edges(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const size_t edge_id = outer_face.repeated_paths()[2].edge_id_at_position(i);
            const double x = scale_x_2.map(static_cast<double>(i + 1));
            const double y = scale_y_2.map(static_cast<double>(i + 1));
            const size_t next_point_index = add_point(x, y, next_node_id);
            add_line(BLUE_RGB, prev_point, next_point_index, edge_id);
            prev_point = next_point_index;
            if (i == outer_face.repeated_paths()[2].number_of_edges() - 1)
                corners[2] = next_point_index;
        }
    }

    EquivalentEmbeddingBuilder(
        const Graph& old_graph,
        const Embedding& old_embedding,
        const Face& outer_face,

        EquivalentEmbedding& equivalent_embedding
    )
        : old_graph(old_graph), old_embedding(old_embedding), outer_face(outer_face),
          graph(equivalent_embedding.graph), embedding(equivalent_embedding.embedding),
          attributes(equivalent_embedding.attributes),
          node_id_to_old(equivalent_embedding.node_id_to_old),
          edge_id_to_old(equivalent_embedding.edge_id_to_old) {}

  public:
    static EquivalentEmbedding
    build(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
        EquivalentEmbedding equivalent_embedding;

        equivalent_embedding.attributes.add_attribute(Attribute::NODES_POSITION);
        equivalent_embedding.attributes.add_attribute(Attribute::NODES_COLOR);
        equivalent_embedding.attributes.add_attribute(Attribute::HIDDEN_NODES);
        equivalent_embedding.attributes.add_attribute(Attribute::EDGES_COLOR);
        equivalent_embedding.attributes.add_attribute(Attribute::HIDDEN_EDGES);

        EquivalentEmbeddingBuilder builder(graph, embedding, outer_face, equivalent_embedding);

        builder.build_border();

        return equivalent_embedding;
    }
};

EquivalentEmbedding
build_equivalent_embedding(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
    return EquivalentEmbeddingBuilder::build(graph, embedding, outer_face);
}

void EquivalentEmbedding::visualize_torus() const {
    TorusMapping mapping;
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (attributes.is_node_hidden(node_id))
            continue;
        if (!attributes.has_position(node_id))
            continue;
        mapping.add_circle(
            Circle2D{
                Point2D{attributes.get_position_x(node_id), attributes.get_position_y(node_id)},
                CIRCLE_RADIUS,
                CIRCLE_DEFAULT_COLOR
            },
            node_id
        );
    }

    for (const EdgeId edge : graph.get_all_edges()) {
        if (attributes.is_edge_hidden(edge.id))
            continue;
        mapping.add_line(
            Line2D{
                attributes.get_position(edge.edge.from_id),
                attributes.get_position(edge.edge.to_id),
                attributes.get_edge_color(edge.id)
            }
        );
    }

    mapping.visualize();
}

} // namespace domus::torus::mapper