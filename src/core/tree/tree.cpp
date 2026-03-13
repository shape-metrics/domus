#include "domus/core/tree/tree.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <print>

#include "../domus_assert.hpp"

Tree::Tree(const size_t root_id) : m_root_id(root_id) { m_node_ids.add_node(root_id); }

void Tree::for_each_node(std::function<void(size_t)> f) const { m_node_ids.for_each(f); }

void Tree::for_each_child(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Tree::for_each_child: node does not exist");
    m_nodeid_to_childrenid.get_neighbors_of_node(node_id).for_each(f);
}

bool Tree::is_root(size_t node_id) const {
    if (!has_node(node_id))
        return false;
    return node_id == m_root_id;
}

bool Tree::has_edge(size_t node_id_1, size_t node_id_2) const {
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return false;
    return m_nodeid_to_childrenid.get_neighbors_of_node(node_id_1).has_node(node_id_2) ||
           m_nodeid_to_childrenid.get_neighbors_of_node(node_id_2).has_node(node_id_1);
}

size_t Tree::get_parent(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Tree::get_parent: node does not exist");
    DOMUS_ASSERT(!is_root(node_id), "Tree::get_parent: node is root");
    return m_nodeid_to_parentid.get(node_id);
}

bool Tree::has_node(size_t id) const { return m_node_ids.has_node(id); }

void Tree::add_node(size_t id, size_t parent_id) {
    DOMUS_ASSERT(!has_node(id), "Tree::add_node: node already exists");
    DOMUS_ASSERT(has_node(parent_id), "Tree::add_node: parent node does not exist");
    m_node_ids.add_node(id);
    m_nodeid_to_parentid.add(id, parent_id);
    m_nodeid_to_childrenid.add_edge(parent_id, id);
}

size_t Tree::add_node(size_t parent_id) {
    while (has_node(m_next_node_id))
        ++m_next_node_id;
    add_node(m_next_node_id, parent_id);
    return m_next_node_id++;
}

size_t Tree::size() const { return m_node_ids.size(); }

std::string Tree::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Tree\n");
    for_each_node([&](size_t node_id) {
        if (is_root(node_id))
            std::format_to(out, "Node {} is root\n", node_id);
        else
            std::format_to(out, "Node {} has parent {}\n", node_id, get_parent(node_id));
        std::format_to(out, "Children of node {}:", node_id);
        for_each_child(node_id, [&](size_t child_id) { std::format_to(out, "  {}", child_id); });
        std::format_to(out, "\n");
    });
    return result;
}

void Tree::print() const { println("{}", to_string()); }