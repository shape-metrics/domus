#include "domus/core/tree/tree.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <cassert>
#include <print>

using namespace std;

Tree::Tree(const size_t root_id) : m_root_id(root_id) { m_node_ids.add_node(root_id); }

const NodesContainer& Tree::get_nodes() const { return m_node_ids; }

bool Tree::is_root(size_t node_id) const {
    if (!has_node(node_id))
        return false;
    return node_id == m_root_id;
}

bool Tree::has_edge(size_t node_id_1, size_t node_id_2) const {
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return false;
    return get_children(node_id_1).has_node(node_id_2) ||
           get_children(node_id_2).has_node(node_id_1);
}

size_t Tree::get_parent(size_t node_id) const {
    assert(has_node(node_id) && "Tree::get_parent: node does not exist");
    assert(!is_root(node_id) && "Tree::get_parent: node is root");
    return m_nodeid_to_parentid.get(node_id);
}

bool Tree::has_node(size_t id) const { return m_node_ids.has_node(id); }

void Tree::add_node(size_t id, size_t parent_id) {
    assert(!has_node(id) && "Node already exists");
    assert(has_node(parent_id) && "Parent node does not exist");
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

const NodesContainer& Tree::get_children(size_t node_id) const {
    assert(has_node(node_id) && "Node does not exist");
    return m_nodeid_to_childrenid.get_neighbors_of_node(node_id);
}

size_t Tree::size() const { return m_node_ids.size(); }

string Tree::to_string() const {
    string str = "Tree\n";
    get_nodes().for_each([&](size_t node_id) {
        if (is_root(node_id))
            str += "Node " + std::to_string(node_id) + " is root\n";
        else
            str += "Node " + std::to_string(node_id) + " has parent " +
                   std::to_string(get_parent(node_id)) + "\n";
        str += "Children of node " + std::to_string(node_id) + ":";
        get_children(node_id).for_each([&str](size_t child_id) {
            str += "  " + std::to_string(child_id);
        });
        str += "\n";
    });
    return str;
}

void Tree::print() const { println("{}", to_string()); }