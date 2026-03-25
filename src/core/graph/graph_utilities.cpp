#include "domus/core/graph/graph_utilities.hpp"

#include "domus/core/graph/graph.hpp"

#include "../domus_debug.hpp"

using namespace domus::graph::utilities;
using namespace domus::graph;

NodesContainer::NodesContainer(const Graph& graph)
    : m_has_node(graph.get_number_of_nodes(), false) {}

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

NodesLabels::NodesLabels(const Graph& graph) { m_labels.resize(graph.get_number_of_nodes()); }

void NodesLabels::add_label(size_t node_id, size_t label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    m_labels[node_id] = label;
}

bool NodesLabels::has_label(size_t node_id) const { return m_labels[node_id].has_value(); }

size_t NodesLabels::get_label(size_t node_id) const {
    DOMUS_ASSERT(
        has_label(node_id),
        "NodesLabels::get_label: node {} does not have a label",
        node_id
    );
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

EdgesLabels::EdgesLabels(const Graph& graph) { m_labels.resize(graph.get_number_of_edges()); }

EdgesLabels::EdgesLabels(size_t number_of_edges) { m_labels.resize(number_of_edges); }

void EdgesLabels::add_label(size_t edge_id, size_t label) {
    DOMUS_ASSERT(!has_label(edge_id), "EdgesLabels::add_label: edge already has a label");
    m_labels[edge_id] = label;
}

bool EdgesLabels::has_label(size_t edge_id) const { return m_labels[edge_id].has_value(); }

size_t EdgesLabels::get_label(size_t edge_id) const {
    DOMUS_ASSERT(
        has_label(edge_id),
        "EdgesLabels::get_label: edge {} does not have a label",
        edge_id
    );
    return *m_labels[edge_id];
}

void EdgesLabels::erase_label(size_t edge_id) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::erase_label: edge does not have a label");
    m_labels[edge_id].reset();
}

void EdgesLabels::update_label(size_t edge_id, size_t new_label) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::update_label: edge does not have a label");
    m_labels[edge_id] = new_label;
}

void EdgesLabels::update_size(size_t edge_id) {
    while (m_labels.size() <= edge_id)
        m_labels.push_back(std::nullopt);
}