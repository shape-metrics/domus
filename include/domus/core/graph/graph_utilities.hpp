#pragma once

#include <deque>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace domus::graph {
class Graph;
}

namespace domus::graph::utilities {

class NodesContainer {
    size_t m_number_of_nodes = 0;
    std::vector<bool> m_has_node;

  public:
    NodesContainer(const Graph& graph);
    void add_node(size_t node_id);
    bool has_node(size_t node_id) const;
    size_t size() const;
    bool empty() const;
    void erase(size_t node_id);
};

class NodesLabels {
    std::vector<std::optional<size_t>> m_labels;

  public:
    NodesLabels(const Graph& graph);
    void add_label(size_t node_id, size_t label);
    bool has_label(size_t node_id) const;
    size_t get_label(size_t node_id) const;
    void erase_label(size_t node_id);
    void update_label(size_t node_id, size_t new_label);
};

class EdgesLabels {
    std::vector<std::optional<size_t>> m_labels;

  public:
    EdgesLabels(const Graph& graph);
    EdgesLabels(size_t number_of_edges);
    void add_label(size_t edge_id, size_t label);
    bool has_label(size_t edge_id) const;
    size_t get_label(size_t edge_id) const;
    void erase_label(size_t edge_id);
    void update_label(size_t edge_id, size_t new_label);
    void update_size(size_t edge_id);
};

class GraphPath {
    std::deque<size_t> m_nodes_ids;
    std::deque<size_t> m_edges_ids;

    std::optional<size_t> m_last_node_id;

  public:
    size_t get_first_node_id() const;
    size_t get_last_node_id() const;
    size_t number_of_edges() const;

    void push_front(const Graph& graph, size_t next_node_id, size_t edge_id);
    void push_back(const Graph& graph, size_t prev_node_id, size_t edge_id);
    void reverse();

    void for_each(std::function<void(size_t, size_t)> f) const; // edge_id, prev_node_id

    auto get_edges() const; // edge_id, prev_node_id

    std::string to_string() const;
    void print() const;
};

inline auto GraphPath::get_edges() const {
    return std::ranges::views::zip(m_edges_ids, m_nodes_ids);
}

} // namespace domus::graph::utilities