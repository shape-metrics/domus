#ifndef MY_TREE_H
#define MY_TREE_H

#include <stddef.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Tree {
    int m_root_id;
    int m_next_node_id = 0;
    std::unordered_set<int> m_node_ids;
    std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_childrenid;
    std::unordered_map<int, int> m_nodeid_to_parentid;

  public:
    Tree(const int root_id);
    const std::unordered_set<int>& get_nodes() const;
    bool is_root(int node_id) const;
    bool has_edge(int node_id_1, int node_id_2) const;
    int get_parent(int node_id) const;
    bool has_node(int id) const;
    void add_node(int id, int parent_id);
    int add_node(int parent_id);
    const std::unordered_set<int>& get_children(int node_id) const;
    size_t size() const;
    std::string to_string() const;
    void print() const;
};

#endif