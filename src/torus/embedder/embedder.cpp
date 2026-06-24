#include "domus/torus/embedder.hpp"

#include <optional>
#include <vector>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/utils.hpp"
#include "domus/ogdf_utils.hpp"

#include "../faces.hpp"
#include "type_2.hpp"
#include "type_3.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

bool extend_embedding(
    Graph& graph, Embedding& embedding, const size_t jolly_id, const std::vector<Face>& faces
) {
    for (const auto& face : faces) {
        if (face.type() == FaceType::TYPE_3) {
            if (handle_type_3(graph, embedding, face, jolly_id))
                return true;
            return false;
        }
    }
    if (handle_type_2(graph, embedding, faces))
        return true;
    return false;
}

std::pair<size_t, size_t>
get_other_edge_id(const Embedding& graph, size_t node_id, size_t neighbor_id) {
    DOMUS_ASSERT(
        graph.get_degree_of_node(node_id) == 2,
        "get_other_neighbor_id: function only for degree 2 nodes"
    );
    std::optional<size_t> other;
    std::optional<size_t> other_edge_id;
    for (const EdgeIter& edge : graph.get_edges(node_id))
        if (edge.neighbor_id != neighbor_id) {
            other = edge.neighbor_id;
            other_edge_id = edge.id;
            break;
        }

    DOMUS_ASSERT(
        other.has_value(),
        "get_other_edge_id: internal error happened, no other neighbor found for node"
    );
    return {*other, *other_edge_id};
}

Embedding remove_added_edges(
    const Graph& graph, const Embedding& embedding, const EdgesLabels<size_t>& subdivided_old_edge
) {
    Embedding clean_embedding(graph);
    const size_t jolly_id = embedding.get_number_of_nodes() - 1;
    for (const size_t node_id : graph.get_nodes_ids()) {
        for (const auto& edge : embedding.get_edges(node_id)) {
            if (edge.neighbor_id == jolly_id)
                continue;
            if (edge.neighbor_id < graph.get_number_of_nodes()) {
                clean_embedding.add_edge(node_id, edge.neighbor_id, edge.id);
            } else {
                const auto [other_node_id, other_edge_id] =
                    get_other_edge_id(embedding, edge.neighbor_id, node_id);
                DOMUS_ASSERT(
                    subdivided_old_edge.get_label(edge.id) ==
                        subdivided_old_edge.get_label(other_edge_id),
                    "remove_added_edges: edge ids do not match correctly"
                );
                clean_embedding
                    .add_edge(node_id, other_node_id, subdivided_old_edge.get_label(edge.id));
            }
        }
    }
    DOMUS_ASSERT(
        graph.get_number_of_nodes() == clean_embedding.get_number_of_nodes() &&
            graph.get_number_of_edges() * 2 == clean_embedding.get_number_of_edges(),
        "remove_added_edges: clean embedding should be equal to original graph"
    );
    DOMUS_ASSERT(
        compute_embedding_genus(clean_embedding) == 1,
        "remove_added_edges: clean embedding should have genus 1"
    );
    return clean_embedding;
}

std::optional<Embedding> compute_toroidal_embedding(const Graph& graph) {
    if (graph.get_number_of_edges() > 3 * graph.get_number_of_nodes())
        return std::nullopt;
    DOMUS_ASSERT(
        algorithms::BiconnectedComponents::compute(graph).get_components().size() == 1,
        "compute_toroidal_embedding: input graph is not biconnected"
    );
    DOMUS_ASSERT(
        algorithms::is_graph_subcubic(graph),
        "compute_toroidal_embedding: input graph is not sub cubic"
    );

    Graph graph_copy = graph;
    EdgesLabels<size_t> subdivided_old_edge;
    for (const size_t edge_id : ogdf_utils::find_kuratowski_subdivision(graph_copy)) {
        Subdivision subdivision = graph_copy.subdivide_edge(edge_id);
        subdivided_old_edge.add_label(subdivision.edge_from_between_id, edge_id);
        subdivided_old_edge.add_label(subdivision.edge_between_to_id, edge_id);
    }

    Embedding embedding(graph_copy);
    for (const size_t edge_id : ogdf_utils::find_kuratowski_subdivision(graph_copy)) {
        Edge edge = graph_copy.get_edge(edge_id);
        embedding.add_edge(edge.from_id, edge.to_id, edge_id);
        embedding.add_edge(edge.to_id, edge.from_id, edge_id);
    }

    // adding jolly node used to insert new paths to split faces
    size_t jolly_id = graph_copy.add_node();
    embedding.add_node();

    std::vector<size_t> degree_3_nodes;
    degree_3_nodes.reserve(6);
    size_t isolated_nodes = 0;
    for (const size_t node_id : embedding.get_nodes_ids()) {
        if (embedding.get_degree_of_node(node_id) == 3)
            degree_3_nodes.push_back(node_id);
        if (embedding.get_degree_of_node(node_id) == 0)
            isolated_nodes++;
    }
    DOMUS_ASSERT(
        degree_3_nodes.size() == 6,
        "compute_toroidal_embedding: expected to find 6 nodes"
    );

    for (auto& combination : domus::utilities::generate_all_bitsets<6>()) {
        for (size_t i = 0; i < 6; i++)
            if (combination.test(i))
                embedding.reverse_circular_order(degree_3_nodes[i]);

        std::vector<Path> paths = compute_faces_in_embedding(graph_copy, embedding);

        if (compute_embedding_genus(
                embedding.get_number_of_nodes() - isolated_nodes,
                embedding.get_number_of_edges() / 2,
                paths.size(),
                1
            ) == 1) {
            std::vector<Face> faces;
            for (Path f : paths)
                faces.push_back(compute_face_from_path(std::move(f), graph_copy));

            if (extend_embedding(graph_copy, embedding, jolly_id, faces))
                return remove_added_edges(graph, embedding, subdivided_old_edge);
        }
        for (size_t i = 0; i < 6; i++)
            if (combination.test(i))
                embedding.reverse_circular_order(degree_3_nodes[i]);
    }

    return std::nullopt;
}

} // namespace domus::torus
