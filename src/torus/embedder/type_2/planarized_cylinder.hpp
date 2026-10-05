#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "adjacencies.hpp"

namespace domus::graph {
class Embedding;
}

namespace domus::torus {

class PlanarizedCylinder {
    size_t face_index;
    graph::Graph graph;
    graph::Embedding embedding;
    graph::utilities::NodesLabels<size_t> node_new_to_old_id;
    graph::utilities::NodesLabels<size_t> node_old_to_new_id;
    graph::utilities::EdgesLabels<size_t> edge_new_to_old_id;
    graph::utilities::EdgesLabels<size_t> edge_old_to_new_id;

    void add_piece(const Bridge& bridge);
    const graph::Path* non_repeated_path_in_component(
        const graph::Graph& component,
        const Face& face,
        const graph::utilities::EdgesLabels<size_t>& component_edge_labels
    );
    bool is_consistent(
        const graph::Graph& component,
        const graph::utilities::NodesLabels<size_t>& component_node_labels,
        const graph::Path& non_repeated_path
    );
    void adjust_rotation_scheme(const Face& face);

  public:
    static std::optional<PlanarizedCylinder>
    build(size_t face_index, const Adjacencies& adjacencies);
    static std::optional<PlanarizedCylinder> build(
        size_t face_index,
        const Adjacencies& adjacencies,
        const std::vector<size_t>& across_ordinary_pieces
    );

    const graph::Graph& get_graph() const;
    const graph::Embedding& get_embedding() const;
    const graph::utilities::NodesLabels<size_t>& get_node_new_to_old_id() const;
    const graph::utilities::NodesLabels<size_t>& get_node_old_to_new_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_new_to_old_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_edge_old_to_new_id() const;

    void merge_into_embedding(
        const Adjacencies& adjacencies, graph::Embedding& destination_embedding
    ) const;
};

} // namespace domus::torus