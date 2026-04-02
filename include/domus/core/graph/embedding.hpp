#pragma once

#include <string>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"

namespace domus::graph {

class Embedding {
    std::vector<std::vector<EdgeIter>> m_adjacency_list;
    utilities::OrientedEdgesLabels<EdgeIter> m_next_in_adjacency_list;
    utilities::OrientedEdgesLabels<EdgeIter> m_prev_in_adjacency_list;
    size_t m_number_of_edges = 0;

  public:
    Embedding(const graph::Graph& graph);

    bool has_node(size_t node_id) const;
    bool are_neighbors(size_t node_1_id, size_t node_2_id) const;

    auto get_nodes_ids() const;

    void add_edge(size_t from_id, size_t to_id, size_t edge_id);

    void add_edge_after(size_t from_id, size_t to_id, size_t edge_id, size_t prev_edge_id);

    void remove_edge(size_t from_id, size_t to_id, size_t edge_id);

    size_t get_degree_of_node(size_t node_id) const;
    EdgeIter next_in_adjacency_list(size_t node_id, size_t neighbor_id, size_t edge_id) const;

    size_t get_number_of_nodes() const;
    size_t get_number_of_edges() const;

    auto get_neighbors(size_t node_id) const;
    auto get_edges(size_t node_id) const;

    bool is_consistent() const;

    std::string to_string() const;
    void print() const;
};

std::vector<graph::Path> compute_faces_in_embedding(const Graph& graph, const Embedding& embedding);

size_t compute_number_of_faces_in_embedding(const Embedding& embedding);

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
);

size_t compute_embedding_genus(const Embedding& embedding);

bool is_embedding_planar(const Embedding& embedding);

inline auto Embedding::get_nodes_ids() const {
    return std::views::iota(size_t{0}, get_number_of_nodes());
}

inline auto Embedding::get_neighbors(size_t node_id) const {
    return std::views::transform(m_adjacency_list[node_id], [&](EdgeIter edge) {
        return edge.neighbor_id;
    });
}

inline auto Embedding::get_edges(size_t node_id) const {
    return std::views::all(m_adjacency_list[node_id]);
}

} // namespace domus::graph