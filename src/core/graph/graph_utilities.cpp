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

template <typename T> void NodesLabels<T>::add_label(size_t node_id, T label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    m_labels[node_id] = std::move(label);
    ++m_number_of_labels;
}

template <typename T> bool NodesLabels<T>::has_label(size_t node_id) const {
    return m_labels[node_id].has_value();
}

template <typename T> const T& NodesLabels<T>::get_label(size_t node_id) const {
    DOMUS_ASSERT(
        has_label(node_id),
        "NodesLabels::get_label: node {} does not have a label",
        node_id
    );
    return m_labels[node_id].value();
}

template <typename T> void NodesLabels<T>::erase_label(size_t node_id) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::erase_label: node does not have a label");
    m_labels[node_id].reset();
    --m_number_of_labels;
}

template <typename T> void NodesLabels<T>::update_label(size_t node_id, T new_label) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::update_label: node does not have a label");
    m_labels[node_id] = std::move(new_label);
}

template <typename T> size_t NodesLabels<T>::get_number_of_labels() const {
    return m_number_of_labels;
}

template <typename T> void NodesLabels<T>::add_or_update_label(size_t node_id, T label) {
    if (has_label(node_id))
        update_label(node_id, std::move(label));
    else
        add_label(node_id, std::move(label));
}

template <typename T> EdgesLabels<T>::EdgesLabels(size_t number_of_edges) {
    m_labels.resize(number_of_edges);
}

template <typename T> void EdgesLabels<T>::add_label(size_t edge_id, T label) {
    DOMUS_ASSERT(!has_label(edge_id), "EdgesLabels::add_label: edge already has a label");
    m_labels[edge_id] = std::move(label);
    ++m_number_of_labels;
}

template <typename T> bool EdgesLabels<T>::has_label(size_t edge_id) const {
    return m_labels[edge_id].has_value();
}

template <typename T> const T& EdgesLabels<T>::get_label(size_t edge_id) const {
    DOMUS_ASSERT(
        has_label(edge_id),
        "EdgesLabels::get_label: edge {} does not have a label",
        edge_id
    );
    return *m_labels[edge_id];
}

template <typename T> void EdgesLabels<T>::erase_label(size_t edge_id) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::erase_label: edge does not have a label");
    m_labels[edge_id].reset();
    --m_number_of_labels;
}

template <typename T> void EdgesLabels<T>::update_label(size_t edge_id, T new_label) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::update_label: edge does not have a label");
    m_labels[edge_id] = std::move(new_label);
}

template <typename T> void EdgesLabels<T>::update_size(size_t edge_id) {
    while (m_labels.size() <= edge_id)
        m_labels.push_back(std::nullopt);
}

template <typename T> bool EdgesLabels<T>::empty() const { return m_number_of_labels == 0; }

template <typename T> size_t EdgesLabels<T>::get_number_of_labels() const {
    return m_number_of_labels;
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

OrientedEdgesContainer::OrientedEdgesContainer(size_t number_of_edges_ids)
    : m_visited_edges_1(number_of_edges_ids), m_visited_edges_2(number_of_edges_ids) {}

bool OrientedEdgesContainer::has_edge(size_t from_id, size_t to_id, size_t edge_id) const {
    if (from_id < to_id)
        return m_visited_edges_1.has_edge(edge_id);
    else
        return m_visited_edges_2.has_edge(edge_id);
}

void OrientedEdgesContainer::add_edge(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.add_edge(edge_id);
    else
        m_visited_edges_2.add_edge(edge_id);
}

void OrientedEdgesContainer::erase(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.erase(edge_id);
    else
        m_visited_edges_2.erase(edge_id);
}

size_t OrientedEdgesContainer::size() const {
    return m_visited_edges_1.size() + m_visited_edges_2.size();
}

bool OrientedEdgesContainer::empty() const { return size() == 0; }

template <typename T>
void OrientedEdgesLabels<T>::add_label(size_t from_id, size_t to_id, size_t edge_id, T label) {
    if (from_id < to_id)
        m_labels_1.add_label(edge_id, std::move(label));
    else
        m_labels_2.add_label(edge_id, std::move(label));
}

template <typename T>
bool OrientedEdgesLabels<T>::has_label(size_t from_id, size_t to_id, size_t edge_id) const {
    if (from_id < to_id)
        return m_labels_1.has_label(edge_id);
    else
        return m_labels_2.has_label(edge_id);
}

template <typename T>
const T& OrientedEdgesLabels<T>::get_label(size_t from_id, size_t to_id, size_t edge_id) const {
    if (from_id < to_id)
        return m_labels_1.get_label(edge_id);
    else
        return m_labels_2.get_label(edge_id);
}

template <typename T>
void OrientedEdgesLabels<T>::erase_label(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_labels_1.erase_label(edge_id);
    else
        m_labels_2.erase_label(edge_id);
}

template <typename T>
void OrientedEdgesLabels<T>::update_label(
    size_t from_id, size_t to_id, size_t edge_id, T new_label
) {
    if (from_id < to_id)
        m_labels_1.update_label(edge_id, std::move(new_label));
    else
        m_labels_2.update_label(edge_id, std::move(new_label));
}

template <typename T> size_t OrientedEdgesLabels<T>::get_number_of_labels() const {
    return m_labels_1.get_number_of_labels() + m_labels_2.get_number_of_labels();
}

template <typename T> bool OrientedEdgesLabels<T>::empty() const {
    return get_number_of_labels() == 0;
}

// Explicit template instantiations for common types
template class NodesLabels<size_t>;
template class EdgesLabels<size_t>;
template class OrientedEdgesLabels<size_t>;

} // namespace domus::graph::utilities