#ifndef MY_GRAPH_UTILITIES_HPP
#define MY_GRAPH_UTILITIES_HPP

#include <functional>
#include <memory>
#include <stddef.h>

#include "domus/core/containers.hpp"

class NodesContainer : protected IntHashSet {
  public:
    void add_node(int node_id);
    bool has_node(int node_id) const;
    int get_one_node_id() const;
    size_t size() const;
    bool empty() const;
    void erase(int node_id);
    void for_each(std::function<void(int)> func) const;
};

class I_AdjacencyList;

class AdjacencyList {
    std::unique_ptr<I_AdjacencyList> m_impl;

  public:
    AdjacencyList();
    ~AdjacencyList();
    AdjacencyList(AdjacencyList&&) noexcept;
    AdjacencyList& operator=(AdjacencyList&&) noexcept;

    void add_edge(int from_id, int to_id);
    bool has_edge(int from_id, int to_id) const;
    const NodesContainer& get_neighbors_of_node(int node_id) const;
    void erase_edge(int from_id, int to_id);
    void erase_node(int node_id);
};

#endif