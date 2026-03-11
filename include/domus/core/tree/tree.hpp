#pragma once

#include <string>

#include "domus/core/containers.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Tree {
    size_t m_root_id;
    size_t m_next_node_id = 0;
    NodesContainer m_node_ids;
    AdjacencyList m_nodeid_to_childrenid;
    Int_ToInt_HashMap m_nodeid_to_parentid;

  public:
    Tree(const size_t root_id);
    const NodesContainer& get_nodes() const;
    bool is_root(size_t node_id) const;
    bool has_edge(size_t node_id_1, size_t node_id_2) const;
    size_t get_parent(size_t node_id) const;
    bool has_node(size_t id) const;
    void add_node(size_t id, size_t parent_id);
    size_t add_node(size_t parent_id);
    const NodesContainer& get_children(size_t node_id) const;
    size_t size() const;
    std::string to_string() const;
    void print() const;
};