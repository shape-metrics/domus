#pragma once

#include <functional>
#include <string>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/path.hpp"

namespace domus::graph {

class Embedding {
    std::vector<std::vector<EdgeIter>> m_adjacency_list;
    std::vector<std::optional<EdgeIter>> m_next_in_adjacency_list_1;
    std::vector<std::optional<EdgeIter>> m_next_in_adjacency_list_2;

    std::vector<std::optional<EdgeIter>> m_prev_in_adjacency_list_1;
    std::vector<std::optional<EdgeIter>> m_prev_in_adjacency_list_2;
    size_t m_number_of_edges = 0;

  public:
    Embedding(const graph::Graph& graph);

    void add_edge(size_t from_id, size_t to_id, size_t edge_id);

    size_t get_node_degree(size_t node_id) const;
    EdgeIter next_in_adjacency_list(size_t node_id, size_t neighbor_id, size_t edge_id) const;
    size_t get_number_of_nodes() const;
    size_t total_number_of_edges() const;

    void for_each_node(std::function<void(size_t)> func) const;

    void for_each_neighbor(size_t node_id, std::function<void(size_t)> func) const;
    void for_each_edge(size_t node_id, std::function<void(EdgeIter)> func) const;

    bool is_consistent() const;

    std::string to_string() const;
    void print() const;
};

std::vector<graph::Path> compute_faces_in_embedding(const Graph& graph, const Embedding& embedding);

size_t compute_number_of_faces_in_embedding(const Graph& graph, const Embedding& embedding);

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
);

size_t compute_embedding_genus(const Graph& graph, const Embedding& embedding);

bool is_embedding_planar(const Graph& graph, const Embedding& embedding);

} // namespace domus::graph