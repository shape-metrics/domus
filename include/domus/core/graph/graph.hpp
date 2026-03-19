#pragma once

#include <functional>
#include <stack>
#include <string>
#include <vector>

#include "domus/core/graph/graph_utilities.hpp"

struct EdgeId {
    size_t id;
    Edge edge;
};

class Graph {
    std::vector<std::vector<size_t>> m_out_adjacency_list;
    std::vector<std::vector<size_t>> m_in_adjacency_list;
    std::vector<std::optional<EdgeId>> m_edges;
    std::stack<size_t> m_free_edges_ids;

  public:
    bool has_node(size_t node_id) const;
    void for_each_node(std::function<void(size_t)> f) const;
    void for_each_out_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_in_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    size_t add_node();
    size_t get_out_degree_of_node(size_t node_id) const;
    size_t get_in_degree_of_node(size_t node_id) const;
    size_t get_degree_of_node(size_t node_id) const;
    void add_edge(size_t from_id, size_t to_id);
    bool has_edge(size_t from_id, size_t to_id) const;
    bool are_neighbors(size_t node_1_id, size_t node_2_id) const;
    size_t size() const;
    size_t get_number_of_edges() const;
    void remove_edge(size_t from_id, size_t to_id);
    std::string to_string(bool undirected) const;
    std::string to_string() const;
    std::string
    to_string(bool undirected, const NodesLabels& labels, const std::string_view name) const;
    void print(bool undirected) const;
};