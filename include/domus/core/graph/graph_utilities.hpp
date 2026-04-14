#pragma once

#include <optional>
#include <vector>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/concept.hpp"

namespace domus::graph::utilities {

class NodesContainer {
    size_t m_number_of_nodes = 0;
    std::vector<bool> m_has_node;

  public:
    template <UndirectedGraphLike G> NodesContainer(const G& graph);
    void add_node(size_t node_id);
    bool has_node(size_t node_id) const;
    size_t size() const;
    bool empty() const;
    void erase(size_t node_id);
};

template <typename T> class NodesLabels {
    std::vector<std::optional<T>> m_labels;
    size_t m_number_of_labels = 0;

  public:
    template <UndirectedGraphLike G> NodesLabels(const G& graph);
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
    template <UndirectedGraphLike G> EdgesLabels(const G& graph);
    EdgesLabels(size_t number_of_edges);
    void add_label(size_t edge_id, T label);
    bool has_label(size_t edge_id) const;
    const T& get_label(size_t edge_id) const;
    void erase_label(size_t edge_id);
    void update_label(size_t edge_id, T new_label);
    void update_size(size_t edge_id);
    bool empty() const;
    size_t get_number_of_labels() const;
};

class EdgesContainer {
    size_t m_number_of_edges = 0;
    std::vector<bool> m_has_edge;

  public:
    template <UndirectedGraphLike G> EdgesContainer(const G& graph);
    EdgesContainer(size_t number_of_edges_ids);
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
    template <UndirectedGraphLike G> OrientedEdgesContainer(const G& graph);
    OrientedEdgesContainer(size_t number_of_edges_ids);
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
    template <UndirectedGraphLike G> OrientedEdgesLabels(const G& graph);
    void add_label(size_t from_id, size_t to_id, size_t edge_id, T label);
    bool has_label(size_t from_id, size_t to_id, size_t edge_id) const;
    const T& get_label(size_t from_id, size_t to_id, size_t edge_id) const;
    void erase_label(size_t from_id, size_t to_id, size_t edge_id);
    void update_label(size_t from_id, size_t to_id, size_t edge_id, T new_label);
    void update_size(size_t edge_id);
    size_t get_number_of_labels() const;
    bool empty() const;
};

// Template specializations

template <UndirectedGraphLike G>
NodesContainer::NodesContainer(const G& graph) : m_has_node(graph.get_number_of_nodes(), false) {}

template <typename T> template <UndirectedGraphLike G> NodesLabels<T>::NodesLabels(const G& graph) {
    m_labels.resize(graph.get_number_of_nodes());
}

template <UndirectedGraphLike G>
EdgesContainer::EdgesContainer(const G& graph) : m_has_edge(graph.get_number_of_edges(), false) {}

template <UndirectedGraphLike G>
OrientedEdgesContainer::OrientedEdgesContainer(const G& graph)
    : m_visited_edges_1(graph), m_visited_edges_2(graph) {}

template <typename T> template <UndirectedGraphLike G> EdgesLabels<T>::EdgesLabels(const G& graph) {
    m_labels.resize(graph.get_number_of_edges());
}

template <typename T>
template <UndirectedGraphLike G>
OrientedEdgesLabels<T>::OrientedEdgesLabels(const G& graph)
    : m_labels_1(graph), m_labels_2(graph) {}

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

template <typename T> void OrientedEdgesLabels<T>::update_size(size_t edge_id) {
    m_labels_1.update_size(edge_id);
    m_labels_2.update_size(edge_id);
}

template <typename T> size_t OrientedEdgesLabels<T>::get_number_of_labels() const {
    return m_labels_1.get_number_of_labels() + m_labels_2.get_number_of_labels();
}

template <typename T> bool OrientedEdgesLabels<T>::empty() const {
    return get_number_of_labels() == 0;
}

} // namespace domus::graph::utilities
