#pragma once

#include <deque>
#include <ranges>
#include <string>

#include "domus/core/graph/graph_utilities.hpp"

namespace domus::graph {

class Graph;

class Path {
    std::deque<size_t> m_nodes_ids;
    std::deque<size_t> m_edges_ids;

    std::optional<size_t> m_last_node_id;

  public:
    size_t get_first_node_id() const;
    size_t get_last_node_id() const;

    size_t get_first_edge_id() const;
    size_t get_last_edge_id() const;

    size_t number_of_edges() const;
    size_t number_of_nodes() const;

    size_t node_id_at_position(size_t position) const;
    size_t edge_id_at_position(size_t position) const;

    bool contains_node_id(size_t node_id) const;
    bool contains_edge_id(size_t edge_id) const;

    void push_front(const Graph& graph, size_t next_node_id, size_t edge_id);
    void push_back(const Graph& graph, size_t prev_node_id, size_t edge_id);
    void reverse();

    void pop_front();
    void pop_back();

    auto get_edges() const; // edge_id, prev_node_id

    std::string to_string() const;
    void print() const;

    bool operator==(const Path& other) const;
};

inline auto Path::get_edges() const { return std::ranges::views::zip(m_edges_ids, m_nodes_ids); }

Path convert_path(
    const Path& path,
    const utilities::NodesLabels<size_t>& node_labels,
    const utilities::EdgesLabels<size_t>& edge_labels,
    const Graph& graph
);

} // namespace domus::graph