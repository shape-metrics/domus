#pragma once

#include <optional>
#include <vector>

#include "domus/core/domus_debug.hpp"

namespace domus::graph::utilities {

/**
 * @brief A container of nodes of a graph. Can add and check nodes
 * in the container in O(1).
 */
class NodesContainer {
    size_t m_number_of_nodes = 0;
    std::vector<bool> m_has_node;

  public:
    /**
     * @brief Adds a node to the container in O(1). Assumes the node is NOT already inside.
     * @param node_id The id of the node to be added.
     */
    void add_node(size_t node_id);
    /**
     * @brief Checks if a node is in the container in O(1).
     * @param node_id The id of the node to be checked.
     */
    bool has_node(size_t node_id) const;
    /**
     * @brief Returns the number of nodes currently in the container in O(1).
     */
    size_t size() const;
    /**
     * @brief Returns if the container is empty in O(1).
     */
    bool empty() const;
    /**
     * @brief Removes a node from the container in O(1). Assumes the node is inside.
     * @param node_id The id of the node to be removed .
     */
    void erase(size_t node_id);
};

/**
 * @brief A class which associates labels with the nodes of a graph.
 * Can add and retrieve labels of nodes in O(1).
 * @tparam T The type of the labels.
 */
template <typename T> class NodesLabels {
    std::vector<std::optional<T>> m_labels;
    size_t m_number_of_labels = 0;

  public:
    /**
     * @brief Adds a label to a node in O(1). The node is assumed to NOT already have a label.
     * @param node_id The id of the node that will receive the label.
     * @param label The label to add to the node.
     */
    void add_label(size_t node_id, T label);
    bool has_label(size_t node_id) const;
    const T& get_label(size_t node_id) const;
    T& get_label(size_t node_id);
    void erase_label(size_t node_id);
    void update_label(size_t node_id, T new_label);
    void add_or_update_label(size_t node_id, T label);
    size_t get_number_of_labels() const;
};

template <typename T> class EdgesLabels {
    std::vector<std::optional<T>> m_labels;
    size_t m_number_of_labels = 0;

  public:
    void add_label(size_t edge_id, T label);
    bool has_label(size_t edge_id) const;
    const T& get_label(size_t edge_id) const;
    void erase_label(size_t edge_id);
    void update_label(size_t edge_id, T new_label);
    bool empty() const;
    size_t get_number_of_labels() const;
};

class EdgesContainer {
    size_t m_number_of_edges = 0;
    std::vector<bool> m_has_edge;

  public:
    void add_edge(size_t edge_id);
    bool has_edge(size_t edge_id) const;
    size_t size() const;
    bool empty() const;
    void erase(size_t edge_id);
};

class OrientedEdgesContainer {
    EdgesContainer m_visited_edges_1;
    EdgesContainer m_visited_edges_2;

  public:
    bool has_edge(size_t from_id, size_t to_id, size_t edge_id) const;
    void add_edge(size_t from_id, size_t to_id, size_t edge_id);
    void erase(size_t from_id, size_t to_id, size_t edge_id);
    size_t size() const;
    bool empty() const;
};

template <typename T> class OrientedEdgesLabels {
    EdgesLabels<T> m_labels_1;
    EdgesLabels<T> m_labels_2;

  public:
    void add_label(size_t from_id, size_t to_id, size_t edge_id, T label);
    bool has_label(size_t from_id, size_t to_id, size_t edge_id) const;
    const T& get_label(size_t from_id, size_t to_id, size_t edge_id) const;
    void erase_label(size_t from_id, size_t to_id, size_t edge_id);
    void update_label(size_t from_id, size_t to_id, size_t edge_id, T new_label);
    size_t get_number_of_labels() const;
    bool empty() const;
};

// Template specializations

template <typename T> void NodesLabels<T>::add_label(size_t node_id, T label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    while (m_labels.size() <= node_id)
        m_labels.push_back(std::nullopt);
    m_labels[node_id] = std::move(label);
    ++m_number_of_labels;
}

template <typename T> bool NodesLabels<T>::has_label(size_t node_id) const {
    if (node_id >= m_labels.size())
        return false;
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

template <typename T> T& NodesLabels<T>::get_label(size_t node_id) {
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

template <typename T> void EdgesLabels<T>::add_label(size_t edge_id, T label) {
    DOMUS_ASSERT(!has_label(edge_id), "EdgesLabels::add_label: edge already has a label");
    while (edge_id >= m_labels.size())
        m_labels.push_back(std::nullopt);
    m_labels[edge_id] = std::move(label);
    ++m_number_of_labels;
}

template <typename T> bool EdgesLabels<T>::has_label(size_t edge_id) const {
    if (edge_id >= m_labels.size())
        return false;
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

template <typename T> bool EdgesLabels<T>::empty() const { return m_number_of_labels == 0; }

template <typename T> size_t EdgesLabels<T>::get_number_of_labels() const {
    return m_number_of_labels;
}

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

} // namespace domus::graph::utilities
