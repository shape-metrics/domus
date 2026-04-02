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

class NodesLabels {
    std::vector<std::optional<size_t>> m_labels;
    size_t m_number_of_labels = 0;

  public:
    template <UndirectedGraphLike G> NodesLabels(const G& graph);
    void add_label(size_t node_id, size_t label);
    bool has_label(size_t node_id) const;
    size_t get_label(size_t node_id) const;
    void erase_label(size_t node_id);
    void update_label(size_t node_id, size_t new_label);
    void add_or_update_label(size_t node_id, size_t label);
    size_t get_number_of_labels() const;
};

class EdgesLabels {
    std::vector<std::optional<size_t>> m_labels;
    size_t m_number_of_labels = 0;

  public:
    template <UndirectedGraphLike G> EdgesLabels(const G& graph);
    EdgesLabels(size_t number_of_edges);
    void add_label(size_t edge_id, size_t label);
    bool has_label(size_t edge_id) const;
    size_t get_label(size_t edge_id) const;
    void erase_label(size_t edge_id);
    void update_label(size_t edge_id, size_t new_label);
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

class OrientedEdgesLabels {
    EdgesLabels m_labels_1;
    EdgesLabels m_labels_2;

  public:
    template <UndirectedGraphLike G> OrientedEdgesLabels(const G& graph);
    void add_label(size_t from_id, size_t to_id, size_t edge_id, size_t label);
    bool has_label(size_t from_id, size_t to_id, size_t edge_id) const;
    size_t get_label(size_t from_id, size_t to_id, size_t edge_id) const;
    void erase_label(size_t from_id, size_t to_id, size_t edge_id);
    void update_label(size_t from_id, size_t to_id, size_t edge_id, size_t new_label);
    size_t get_number_of_labels() const;
    bool empty() const;
};

template <UndirectedGraphLike G>
NodesContainer::NodesContainer(const G& graph) : m_has_node(graph.get_number_of_nodes(), false) {}

template <UndirectedGraphLike G> NodesLabels::NodesLabels(const G& graph) {
    m_labels.resize(graph.get_number_of_nodes());
}

template <UndirectedGraphLike G>
EdgesContainer::EdgesContainer(const G& graph) : m_has_edge(graph.get_number_of_edges(), false) {}

template <UndirectedGraphLike G>
OrientedEdgesContainer::OrientedEdgesContainer(const G& graph)
    : m_visited_edges_1(graph), m_visited_edges_2(graph) {}

template <UndirectedGraphLike G> EdgesLabels::EdgesLabels(const G& graph) {
    m_labels.resize(graph.get_number_of_edges());
}

template <UndirectedGraphLike G>
OrientedEdgesLabels::OrientedEdgesLabels(const G& graph) : m_labels_1(graph), m_labels_2(graph) {}

} // namespace domus::graph::utilities