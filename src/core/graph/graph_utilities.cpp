#include "domus/core/graph/graph_utilities.hpp"

#include "../domus_assert.hpp"
#include "domus/core/graph/graph.hpp"

NodesContainer::NodesContainer(const Graph& graph) : m_has_node(graph.size(), false) {}

void NodesContainer::add_node(size_t node_id) {
    DOMUS_ASSERT(!has_node(node_id), "NodesContainer::add_node: node already exists");
    m_has_node[node_id] = true;
    m_number_of_nodes++;
}

bool NodesContainer::has_node(size_t node_id) const { return m_has_node.at(node_id); }

size_t NodesContainer::size() const { return m_number_of_nodes; }

bool NodesContainer::empty() const { return size() == 0; }

void NodesContainer::erase(size_t node_id) {
    DOMUS_ASSERT(has_node(node_id), "NodesContainer::erase: node does not exist");
    m_has_node[node_id] = false;
    m_number_of_nodes--;
}

NodesLabels::NodesLabels(const Graph& graph) { m_labels.resize(graph.size()); }

NodesLabels::NodesLabels(size_t size) { m_labels.resize(size); }

void NodesLabels::add_label(size_t node_id, size_t label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    m_labels[node_id] = label;
}

bool NodesLabels::has_label(size_t node_id) const { return m_labels[node_id].has_value(); }

size_t NodesLabels::get_label(size_t node_id) const {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::get_label: node does not have a label");
    return m_labels[node_id].value();
}

void NodesLabels::erase_label(size_t node_id) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::erase_label: node does not have a label");
    m_labels[node_id].reset();
}

void NodesLabels::update_label(size_t node_id, size_t new_label) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::update_label: node does not have a label");
    m_labels[node_id] = new_label;
}
