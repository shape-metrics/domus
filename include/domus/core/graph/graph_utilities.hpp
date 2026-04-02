#pragma once

#include <optional>
#include <vector>

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

} // namespace domus::graph::utilities