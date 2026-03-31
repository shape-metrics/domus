#include "domus/core/graph/graph_utilities.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph::utilities {
using namespace domus::graph;

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

void NodesLabels::add_label(size_t node_id, size_t label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    m_labels[node_id] = label;
    ++m_number_of_labels;
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
    --m_number_of_labels;
}

void NodesLabels::update_label(size_t node_id, size_t new_label) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::update_label: node does not have a label");
    m_labels[node_id] = new_label;
}

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

EdgesContainer::EdgesContainer(size_t number_of_edges_ids)
    : m_has_edge(number_of_edges_ids, false) {}

void EdgesContainer::add_edge(size_t edge_id) {
    DOMUS_ASSERT(!has_edge(edge_id), "EdgesContainer::add_edge: edge already exists");
    m_has_edge[edge_id] = true;
    m_number_of_edges++;
}

bool EdgesContainer::has_edge(size_t edge_id) const { return m_has_edge.at(edge_id); }

size_t EdgesContainer::size() const { return m_number_of_edges; }

bool EdgesContainer::empty() const { return size() == 0; }

void EdgesContainer::erase(size_t edge_id) {
    DOMUS_ASSERT(has_edge(edge_id), "EdgesContainer::erase: edge does not exist");
    m_has_edge[edge_id] = false;
    m_number_of_edges--;
}

VisitedEdges::VisitedEdges(size_t number_of_edges_ids)
    : m_visited_edges_1(number_of_edges_ids), m_visited_edges_2(number_of_edges_ids) {}

bool VisitedEdges::has_edge(size_t from_id, size_t to_id, size_t edge_id) const {
    if (from_id < to_id)
        return m_visited_edges_1.has_edge(edge_id);
    else
        return m_visited_edges_2.has_edge(edge_id);
}

void VisitedEdges::add_edge(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.add_edge(edge_id);
    else
        m_visited_edges_2.add_edge(edge_id);
}

void VisitedEdges::erase(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.erase(edge_id);
    else
        m_visited_edges_2.erase(edge_id);
}

size_t VisitedEdges::size() const { return m_visited_edges_1.size() + m_visited_edges_2.size(); }

bool VisitedEdges::empty() const { return size() == 0; }

} // namespace domus::graph::utilities