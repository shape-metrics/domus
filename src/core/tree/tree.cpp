#include "domus/core/tree/tree.hpp"

#include <print>
#include <queue>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "../domus_assert.hpp"

void Tree::for_each_node(std::function<void(size_t)> f) const {
    for (size_t node_id = 0; node_id < size(); ++node_id)
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
    return m_nodeid_to_parentid.at(node_id);
}

bool Tree::has_node(size_t id) const { return id < size(); }

size_t Tree::add_node(size_t parent_id) {
    DOMUS_ASSERT(has_node(parent_id), "Tree::add_node: parent node does not exist");
    size_t new_node_id = size();
    m_nodeid_to_parentid.push_back(parent_id);
    m_nodeid_to_childrenid.push_back({});
    m_nodeid_to_childrenid.at(parent_id).push_back(new_node_id);
    return new_node_id;
}

size_t Tree::size() const { return m_nodeid_to_parentid.size(); }

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

std::optional<Tree> Tree::build_spanning_tree(const Graph& graph) {
    if (graph.size() <= 1)
        return std::nullopt;
    NodesLabels parent(graph);
    std::queue<size_t> queue;
    queue.push(0u);
    parent.add_label(queue.front(), graph.size());
    size_t number_visited_nodes = 1;
    while (!queue.empty()) {
        size_t node_id = queue.front();
        queue.pop();
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!parent.has_label(neighbor_id)) {
                parent.add_label(neighbor_id, node_id);
                ++number_visited_nodes;
                queue.push(neighbor_id);
            }
        });
    }
    if (number_visited_nodes != graph.size())
        return std::nullopt;
    Tree tree;
    tree.m_nodeid_to_childrenid.resize(graph.size());
    tree.m_nodeid_to_parentid.reserve(graph.size());
    tree.m_nodeid_to_parentid.push_back(graph.size());
    for (size_t node_id = 1; node_id < graph.size(); ++node_id) {
        tree.m_nodeid_to_parentid.push_back(parent.get_label(node_id));
        tree.m_nodeid_to_childrenid.at(parent.get_label(node_id)).push_back(node_id);
    }
    return tree;
}