#include "domus/core/tree/tree.hpp"

#include <cassert>
#include <initializer_list>
#include <iostream>

using namespace std;

Tree::Tree(const int root_id) : m_root_id(root_id) { m_node_ids.insert(root_id); }

const unordered_set<int>& Tree::get_nodes() const { return m_node_ids; }

bool Tree::is_root(int node_id) const {
    if (!has_node(node_id))
        return false;
    return node_id == m_root_id;
}

bool Tree::has_edge(int node_id_1, int node_id_2) const {
    if (!has_node(node_id_1) || !has_node(node_id_2))
        return false;
    return get_children(node_id_1).contains(node_id_2) ||
           get_children(node_id_2).contains(node_id_1);
}

expected<int, string> Tree::get_parent(int node_id) const {
    if (!has_node(node_id)) {
        string error_msg =
            "Error in Tree::get_parent: node " + std::to_string(node_id) + " does not exist";
        return std::unexpected(error_msg);
    }
    if (is_root(node_id)) {
        string error_msg =
            "Error in Tree::get_parent: node " + std::to_string(node_id) + " is root";
        return std::unexpected(error_msg);
    }
    return m_nodeid_to_parentid.at(node_id);
}

bool Tree::has_node(int id) const { return m_node_ids.contains(id); }

void Tree::add_node(int id, int parent_id) {
    assert(!has_node(id) && "Node already exists");
    assert(has_node(parent_id) && "Parent node does not exist");
    m_node_ids.insert(id);
    m_nodeid_to_parentid[id] = parent_id;
    m_nodeid_to_childrenid[parent_id].insert(id);
    m_nodeid_to_childrenid[id] = {};
}

int Tree::add_node(int parent_id) {
    while (has_node(m_next_node_id))
        ++m_next_node_id;
    add_node(m_next_node_id, parent_id);
    return m_next_node_id++;
}

const unordered_set<int>& Tree::get_children(int node_id) const {
    assert(has_node(node_id) && "Node does not exist");
    return m_nodeid_to_childrenid.at(node_id);
}

size_t Tree::size() const { return m_node_ids.size(); }

string Tree::to_string() const {
    string str = "Tree\n";
    for (int node_id : m_node_ids) {
        if (is_root(node_id))
            str += "Node " + std::to_string(node_id) + " is root\n";
        else
            str += "Node " + std::to_string(node_id) + " has parent " +
                   std::to_string(*get_parent(node_id)) + "\n";
        str += "Children of node " + std::to_string(node_id) + ":";
        for (int child_id : get_children(node_id))
            str += "  " + std::to_string(child_id);
        str += "\n";
    }
    return str;
}

void Tree::print() const { std::cout << to_string(); }