#pragma once

#include <stdint.h>
#include <string>

#include "domus/core/graph/graph_utilities.hpp"

class DirectedGraph {
    int m_next_node_id = 0;
    size_t m_total_edges = 0;
    NodesContainer m_nodes_ids;
    AdjacencyList m_out_adjacency_list;
    AdjacencyList m_in_adjacency_list;

  public:
    bool has_node(int node_id) const;
    void for_each_node(std::function<void(int)> f) const;
    const NodesContainer& get_out_neighbors_of_node(int node_id) const;
    const NodesContainer& get_in_neighbors_of_node(int node_id) const;
    void add_node(int id);
    int add_node();
    size_t get_out_degree_of_node(int node_id) const;
    size_t get_in_degree_of_node(int node_id) const;
    void add_edge(int from_id, int to_id);
    bool has_edge(int from_id, int to_id) const;
    bool are_neighbors(int node_1_id, int node_2_id) const;
    size_t size() const;
    size_t get_number_of_edges() const;
    void remove_node(int node_id);
    void remove_edge(int from_id, int to_id);
    std::string to_string() const;
    void print() const;
};

class UndirectedGraph {
    int m_next_node_id = 0;
    size_t m_total_edges = 0;
    NodesContainer m_nodes_ids;
    AdjacencyList m_adjacency_list;

  public:
    int get_one_node_id() const;
    bool has_node(int node_id) const;
    void for_each_node(std::function<void(int)> f) const;
    const NodesContainer& get_neighbors_of_node(int node_id) const;
    void add_node(int id);
    int add_node();
    size_t get_degree_of_node(int node_id) const;
    void add_edge(int node_1_id, int node_2_id);
    bool has_edge(int node_1_id, int node_2_id) const;
    size_t size() const;
    size_t get_number_of_edges() const;
    void remove_node(int node_id);
    void remove_edge(int node_1_id, int node_2_id);
    std::string to_string() const;
    void print() const;
};