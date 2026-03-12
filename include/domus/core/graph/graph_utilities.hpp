#pragma once

#include <functional>
#include <memory>

#include "domus/core/containers.hpp"

class NodesContainer : protected IntHashSet {
  public:
    void add_node(size_t node_id);
    bool has_node(size_t node_id) const;
    size_t get_one_node_id() const;
    size_t size() const;
    bool empty() const;
    void erase(size_t node_id);
    void for_each(std::function<void(size_t)> func) const;
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

class I_AdjacencyList;

class AdjacencyList {
    std::unique_ptr<I_AdjacencyList> m_impl;

  public:
    AdjacencyList();
    ~AdjacencyList();
    AdjacencyList(AdjacencyList&&) noexcept;
    AdjacencyList& operator=(AdjacencyList&&) noexcept;

    void add_edge(size_t from_id, size_t to_id);
    bool has_edge(size_t from_id, size_t to_id) const;
    const NodesContainer& get_neighbors_of_node(size_t node_id) const;
    void erase_edge(size_t from_id, size_t to_id);
    void erase_node(size_t node_id);
};