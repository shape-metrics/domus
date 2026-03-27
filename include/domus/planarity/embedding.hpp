#pragma once

#include <functional>
#include <string>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/path.hpp"

// TODO maybe move this stuff elsewhere??
namespace domus::planarity {

// TODO add edge_ids
class Embedding {
    const graph::Graph& m_graph;
    std::vector<std::vector<size_t>> adjacency_list;
    size_t number_of_edges_m = 0;

  public:
    Embedding(const graph::Graph& graph);
    void add_edge(size_t from_id, size_t to_id);
    size_t get_node_degree(size_t node_id) const;
    size_t next_element_in_adjacency_list(size_t node_id, size_t element) const;
    void for_each_node(std::function<void(size_t)> func) const;
    void for_each_neighbor(size_t node_id, std::function<void(size_t)> func) const;
    std::string to_string() const;
    size_t size() const;
    size_t total_number_of_edges() const;
    bool is_consistent() const;
    const graph::Graph& get_graph() const;
    void print() const;
};

// TODO
std::vector<graph::Path> compute_faces_in_embedding(const Embedding& embedding);

size_t compute_number_of_faces_in_embedding(const Embedding& embedding);

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
);

size_t compute_embedding_genus(const Embedding& embedding);

bool is_embedding_planar(const Embedding& embedding);

} // namespace domus::planarity