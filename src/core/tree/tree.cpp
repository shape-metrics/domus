#include "domus/core/tree/tree.hpp"

#include <print>

#include "../domus_debug.hpp"

namespace domus::tree {

void Tree::for_each_node(std::function<void(size_t)> f) const {
    for (size_t node_id = 0; node_id < get_number_of_nodes(); ++node_id)
        f(node_id);
}

void Tree::for_each_child(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Tree::for_each_child: node does not exist");
    for (size_t child_id : m_nodeid_to_childrenid.at(node_id))
        f(child_id);
}

bool Tree::is_root(size_t node_id) const { return node_id == 0; }

bool Tree::has_edge(size_t node_id_1, size_t node_id_2) const {
    DOMUS_ASSERT(node_id_1 != node_id_2, "Tree::has_edge: elements are equal");
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return false;
    if (is_root(node_id_1)) {
        return get_parent(node_id_2) == node_id_1;
    } else if (is_root(node_id_2)) {
        return get_parent(node_id_1) == node_id_2;
    } else {
        return get_parent(node_id_2) == node_id_1 || get_parent(node_id_1) == node_id_2;
    }
}

size_t Tree::get_parent(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Tree::get_parent: node does not exist");
    DOMUS_ASSERT(!is_root(node_id), "Tree::get_parent: node is root");
    DOMUS_ASSERT(
        m_nodeid_to_parentid.at(node_id).has_value(),
        "Tree::get_parent: node has no parent yet"
    );
    return *m_nodeid_to_parentid.at(node_id);
}

bool Tree::has_node(size_t id) const { return id < get_number_of_nodes(); }

size_t Tree::add_node(size_t parent_id) {
    DOMUS_ASSERT(has_node(parent_id), "Tree::add_node: parent node does not exist");
    size_t new_node_id = get_number_of_nodes();
    m_nodeid_to_parentid.push_back(parent_id);
    m_nodeid_to_childrenid.push_back({});
    m_nodeid_to_childrenid.at(parent_id).push_back(new_node_id);
    return new_node_id;
}

size_t Tree::get_number_of_nodes() const { return m_nodeid_to_parentid.size(); }

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

size_t Tree::add_node() {
    m_nodeid_to_childrenid.push_back({});
    m_nodeid_to_parentid.push_back(std::nullopt);
    return get_number_of_nodes() - 1;
}

bool Tree::has_parent(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Tree::has_parent: node does not exist");
    return m_nodeid_to_parentid.at(node_id).has_value();
}

void Tree::set_parent(size_t child_id, size_t parent_id) {
    DOMUS_ASSERT(has_node(child_id), "Tree::set_parent: child node does not exist");
    DOMUS_ASSERT(has_node(parent_id), "Tree::set_parent: parent node does not exist");
    DOMUS_ASSERT(!has_parent(child_id), "Tree::set_parent: child node already has a parent");
    m_nodeid_to_parentid.at(child_id) = parent_id;
    m_nodeid_to_childrenid.at(parent_id).push_back(child_id);
}

} // namespace domus::tree