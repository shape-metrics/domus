#ifndef MY_TREE_H
#define MY_TREE_H

#include <expected>
#include <stddef.h>
#include <string>

#include "domus/core/containers.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Tree {
    int m_root_id;
    int m_next_node_id = 0;
    NodesContainer m_node_ids;
    AdjacencyList m_nodeid_to_childrenid;
    Int_ToInt_HashMap m_nodeid_to_parentid;

  public:
    Tree(const int root_id);
    const NodesContainer& get_nodes() const;
    bool is_root(int node_id) const;
    bool has_edge(int node_id_1, int node_id_2) const;
    std::expected<int, std::string> get_parent(int node_id) const;
    bool has_node(int id) const;
    void add_node(int id, int parent_id);
    int add_node(int parent_id);
    const NodesContainer& get_children(int node_id) const;
    size_t size() const;
    std::string to_string() const;
    void print() const;
};

#endif