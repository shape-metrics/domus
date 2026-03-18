#pragma once

#include <cstddef>
#include <optional>
#include <vector>

class Graph;

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
    NodesLabels(size_t size);
    void add_label(size_t node_id, size_t label);
    bool has_label(size_t node_id) const;
    size_t get_label(size_t node_id) const;
    void erase_label(size_t node_id);
    void update_label(size_t node_id, size_t new_label);
};

struct Edge {
    size_t from_id;
    size_t to_id;
    bool operator==(const Edge& other) const {
        return from_id == other.from_id && to_id == other.to_id;
    }
};

struct edge_hash {
    size_t operator()(const Edge& edge) const {
        size_t h1 = std::hash<size_t>{}(edge.from_id);
        size_t h2 = std::hash<size_t>{}(edge.to_id);
        size_t mult = h2 * 0x9e3779b9;
        return h1 ^ (mult + (h1 << 6) + (h1 >> 2));
    }
};