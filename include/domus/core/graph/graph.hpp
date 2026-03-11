#pragma once

#include <string>

#include "domus/core/graph/graph_utilities.hpp"

class Graph {
    size_t m_next_node_id = 0;
    size_t m_total_edges = 0;
    NodesContainer m_nodes_ids;
    AdjacencyList m_out_adjacency_list;
    AdjacencyList m_in_adjacency_list;

  public:
    size_t get_one_node_id() const;
    bool has_node(size_t node_id) const;
    void for_each_node(std::function<void(size_t)> f) const;
    void for_each_out_neighbors(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_in_neighbors(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    void add_node(size_t id);
    size_t add_node();
    size_t get_out_degree_of_node(size_t node_id) const;
    size_t get_in_degree_of_node(size_t node_id) const;
    size_t get_degree_of_node(size_t node_id) const;
    void add_edge(size_t from_id, size_t to_id);
    bool has_edge(size_t from_id, size_t to_id) const;
    bool are_neighbors(size_t node_1_id, size_t node_2_id) const;
    size_t size() const;
    size_t get_number_of_edges() const;
    void remove_node(size_t node_id);
    void remove_edge(size_t from_id, size_t to_id);
    std::string to_string() const;
    void print() const;
};