#pragma once

#include <optional>
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

} // namespace domus::graph::utilities