#ifndef MY_GRAPH_H
#define MY_GRAPH_H

#include <stddef.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "domus/core/utils.hpp"

using GraphEdgeHashSet = std::unordered_set<std::pair<int, int>, int_pair_hash>;

template <typename T>
using GraphEdgeHashMap = std::unordered_map<std::pair<int, int>, T, int_pair_hash>;

class DirectedGraph {
    int m_next_node_id = 0;
    size_t m_total_edges = 0;
    std::unordered_set<int> m_nodes_ids;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_out_neighbors_ids;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_in_neighbors_ids;

  public:
    bool has_node(int node_id) const;
    const std::unordered_set<int>& get_nodes_ids() const;
    const std::unordered_set<int>& get_out_neighbors_of_node(int node_id) const;
    const std::unordered_set<int>& get_in_neighbors_of_node(int node_id) const;
    void add_node(int id);
    int add_node();
    size_t get_out_degree_of_node(int node_id) const;
    size_t get_in_degree_of_node(int node_id) const;
    void add_edge(int from_id, int to_id);
    bool has_edge(int from_id, int to_id) const;
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
    std::unordered_set<int> m_nodes_ids;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_neighbors_ids;

  public:
    bool has_node(int node_id) const;
    const std::unordered_set<int>& get_nodes_ids() const;
    const std::unordered_set<int>& get_neighbors_of_node(int node_id) const;
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

#endif
