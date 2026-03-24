#pragma once

#include <functional>
#include <string>

#include "domus/core/graph/graph_utilities.hpp"

namespace domus::graph {

class Cycle {
    friend class Graph;
    std::vector<size_t> m_nodes_ids;
    std::vector<size_t> m_edges_ids;

  public:
    Cycle(const utilities::GraphPath& path);
    bool empty() const;
    size_t size() const;
    // void insert(size_t index, size_t node_id);
    bool has_node(size_t node_id) const;
    bool has_edge_id(size_t edge_id) const;
    size_t node_position(size_t node_id) const;
    size_t node_id_at(size_t index) const;
    size_t edge_id_at(size_t index) const;
    void for_each(std::function<void(size_t)> func) const;
    std::string to_string() const;
    void print() const;
};

} // namespace domus::graph