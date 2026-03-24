#pragma once

#include <functional>
#include <optional>
#include <ranges>
#include <string>

namespace domus::tree {

// node 0 is always root
class Tree {
    std::vector<std::vector<size_t>> m_nodeid_to_childrenid{{}};
    std::vector<std::optional<size_t>> m_nodeid_to_parentid{std::nullopt};

  public:
    Tree() = default;
    size_t add_node();
    size_t add_node(size_t parent_id);
    void set_parent(size_t child_id, size_t parent_id);

    size_t get_parent(size_t node_id) const;
    bool has_parent(size_t node_id) const;
    bool has_node(size_t id) const;
    bool is_root(size_t node_id) const;
    bool has_edge(size_t node_id_1, size_t node_id_2) const;
    size_t get_number_of_nodes() const;

    void for_each_node(std::function<void(size_t)> f) const;
    void for_each_child(size_t node_id, std::function<void(size_t)> f) const;

    auto get_node_ids() const;

    auto get_children(size_t node_id) const;

    std::string to_string() const;
    void print() const;
};

inline auto Tree::get_node_ids() const {
    return std::views::iota(size_t{0}, get_number_of_nodes());
}

inline auto Tree::get_children(size_t node_id) const {
    return std::views::all(m_nodeid_to_childrenid[node_id]);
}

} // namespace domus::tree