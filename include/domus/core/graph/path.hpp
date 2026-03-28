#pragma once

#include <deque>
#include <functional>
#include <ranges>
#include <string>

namespace domus::graph {

class Graph;

class Path {
    std::deque<size_t> m_nodes_ids;
    std::deque<size_t> m_edges_ids;

    std::optional<size_t> m_last_node_id;

  public:
    size_t get_first_node_id() const;
    size_t get_last_node_id() const;
    size_t number_of_edges() const;

    size_t node_id_at_position(size_t position) const;
    size_t edge_id_at_position(size_t position) const;

    void push_front(const Graph& graph, size_t next_node_id, size_t edge_id);
    void push_back(const Graph& graph, size_t prev_node_id, size_t edge_id);
    void reverse();

    void for_each(std::function<void(size_t, size_t)> f) const; // edge_id, prev_node_id

    auto get_edges() const; // edge_id, prev_node_id

    std::string to_string() const;
    void print() const;
};

inline auto Path::get_edges() const { return std::ranges::views::zip(m_edges_ids, m_nodes_ids); }

} // namespace domus::graph