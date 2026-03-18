#pragma once

#include <functional>
#include <optional>
#include <string>

class Graph;

// node 0 is always root
class Tree {
    std::vector<std::vector<size_t>> m_nodeid_to_childrenid;
    std::vector<size_t> m_nodeid_to_parentid;

  public:
    void for_each_node(std::function<void(size_t)> f) const;
    bool is_root(size_t node_id) const;
    bool has_edge(size_t node_id_1, size_t node_id_2) const;
    size_t get_parent(size_t node_id) const;
    bool has_node(size_t id) const;
    size_t add_node(size_t parent_id);
    void for_each_child(size_t node_id, std::function<void(size_t)> f) const;
    size_t size() const;
    std::string to_string() const;
    void print() const;
    static std::optional<Tree> build_spanning_tree(const Graph& graph);
};