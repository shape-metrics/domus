#include "embedding_converter.hpp"

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/drawing/linear_scale.hpp"

namespace domus::torus::mapper {
using namespace domus::graph;
using namespace domus::graph::utilities;
using namespace domus::drawing;
using color::ColorRGB;

class EquivalentEmbeddingBuilder {
    const Graph& old_graph;
    const Embedding& old_embedding;
    const Face& outer_face;

    Graph& graph;
    Embedding& embedding;
    Attributes& attributes;
    NodesLabels<size_t>& node_id_to_old;
    EdgesLabels<size_t>& edge_id_to_old;

    size_t add_point(double x, double y, size_t node_id) {
        const size_t new_node_id = embedding.add_node();
        DOMUS_ASSERT(
            new_node_id < graph.get_number_of_nodes(),
            "EquivalentEmbeddingBuilder::add_point: added too many points"
        );
        attributes.set_position(new_node_id, x, y);
        node_id_to_old.add_label(new_node_id, node_id);
        return new_node_id;
    }

    size_t add_line(ColorRGB color, size_t node_id_1, size_t node_id_2, size_t old_edge_id) {
        const size_t edge_id = graph.add_edge(node_id_1, node_id_2);
        attributes.set_edge_color(edge_id, color);
        edge_id_to_old.add_label(edge_id, old_edge_id);
        return edge_id;
    }

    void initialize_border() {
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
        const size_t first_point = prev_point;

        for (size_t i = 0; i < outer_face.repeated_paths()[0].number_of_edges(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[0].node_id_at_position(i + 1);
            const size_t edge_id = outer_face.repeated_paths()[0].edge_id_at_position(i);
            const double x = scale.map(static_cast<double>(i + 1));
            const size_t next_point_index = add_point(x, 0.0, next_node_id);
            add_line(GREEN_RGB, prev_point, next_point_index, edge_id);
            prev_point = next_point_index;
        }
        std::optional<size_t> last_point_second_path;
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
                last_point_second_path = next_point_index;
        }

        // adding the third repeated path (left diagonal)
        const double last_x =
            scale.map(static_cast<double>(outer_face.repeated_paths()[0].number_of_edges()));
        prev_point = first_point;
        const ScaleLinear scale_x(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            last_x,
            true
        );
        const ScaleLinear scale_y(
            0.0,
            static_cast<double>(outer_face.repeated_paths()[2].number_of_nodes() - 1),
            0.0,
            1.0,
            true
        );
        for (size_t i = 0; i < outer_face.repeated_paths()[2].number_of_nodes(); i++) {
            const size_t next_node_id = outer_face.repeated_paths()[2].node_id_at_position(i + 1);
            const size_t edge_id = outer_face.repeated_paths()[2].edge_id_at_position(i);
            const double x = scale.map(static_cast<double>(i + 1));
            const double y = scale_y.map(static_cast<double>(i + 1));
            const size_t next_point_index = add_point(x, y, next_node_id);
            add_line(BLUE_RGB, prev_point, next_point_index, edge_id);
            prev_point = next_point_index;
        }
    }

    void build() {
        //
    }

  public:
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
          edge_id_to_old(equivalent_embedding.edge_id_to_old) {
        initialize_border();
        build();
    }
};

EquivalentEmbedding
build_equivalent_embedding(const Graph& graph, const Embedding& embedding, const Face& outer_face) {
    EquivalentEmbedding equivalent_embedding;
    for (size_t i = 0; i < graph.get_number_of_nodes(); i++)
        equivalent_embedding.graph.add_node();
    for (auto& path : outer_face.repeated_paths())
        for (size_t i = 0; i < path.number_of_nodes() - 2; ++i)
            equivalent_embedding.graph.add_node();
    auto faces = compute_faces_in_embedding(graph, embedding);
    size_t number_of_wheels = 0;
    for (auto& face : faces)
        if (face.number_of_edges() > 3)
            number_of_wheels++;
    std::vector<size_t> wheels_nodes_id;
    for (size_t i = 0; i < number_of_wheels; i++)
        wheels_nodes_id.push_back(equivalent_embedding.graph.add_node());

    equivalent_embedding.attributes.add_attribute(Attribute::NODES_POSITION);

    EquivalentEmbeddingBuilder builder(graph, embedding, outer_face, equivalent_embedding);

    return equivalent_embedding;

    // for (const auto edge : graph.get_all_edges())
    //     new_edge_id_to_old.add_label(
    //         new_graph.add_edge(edge.edge.from_id, edge.edge.to_id),
    //         edge.id
    //     );
    // for (const size_t node_id : graph.get_nodes_ids())
    //     for (auto edge : embedding.get_edges(node_id))
    //         new_embedding
    //             .add_edge(node_id, edge.neighbor_id, old_edge_id_to_new.get_label(edge.id));

    // for (auto& face : faces) {
    //     if (face.number_of_edges() <= 3)
    //         continue;
    //     const size_t wheel_center_id = wheels_nodes_id.back();
    //     wheels_nodes_id.pop_back();
    //     std::vector<size_t> wheel_edges_id;
    //     for (size_t i = 0; i < face.number_of_edges(); ++i)
    //         wheel_edges_id.push_back(
    //             new_graph.add_edge(wheel_center_id, face.node_id_at_position(i))
    //         );
    //     for (size_t i = 0; i < face.number_of_edges(); ++i) {
    //         const size_t node_id = face.node_id_at_position(i);
    //         const size_t edge_id = face.edge_id_at_position(i);
    //         new_embedding.add_edge_before(node_id, wheel_center_id, wheel_edges_id[i], edge_id);
    //         new_embedding.add_edge(wheel_center_id, node_id, wheel_edges_id[i]);
    //     }
    //     new_embedding.reverse_circular_order(wheel_center_id);
    // }
}

} // namespace domus::torus::mapper