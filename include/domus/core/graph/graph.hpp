#pragma once

#include <functional>
#include <ranges>
#include <stack>
#include <string>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::graph {

struct Edge {
    size_t from_id;
    size_t to_id;
    bool operator==(const Edge& other) const {
        return from_id == other.from_id && to_id == other.to_id;
    }
};

struct EdgeId {
    size_t id;
    Edge edge;
};

struct Subdivision {
    const size_t from_id;
    const size_t in_between_id;
    const size_t to_id;
    const size_t edge_from_between_id;
    const size_t edge_between_to_id;
};

class Graph {
    std::vector<std::vector<size_t>> m_out_adjacency_list{};
    std::vector<std::vector<size_t>> m_in_adjacency_list{};
    std::vector<std::optional<EdgeId>> m_edges{};
    std::stack<size_t> m_free_edges_ids{};

  public:
    size_t add_node();
    size_t add_edge(size_t from_id, size_t to_id);
    size_t remove_edge(size_t from_id, size_t to_id);
    Subdivision subdivide_edge(size_t edge_id);
    void remove_edge(size_t edge_id);

    bool add_subdivision_to_cycle(const Subdivision& subdivision, Cycle& cycle) const;

    void for_each_node(std::function<void(size_t)> f) const;

    auto get_node_ids() const;

    void for_each_out_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_in_neighbor(size_t node_id, std::function<void(size_t)> f) const;
    void for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const;

    auto get_out_neighbors(size_t node_id) const;
    auto get_in_neighbors(size_t node_id) const;
    auto get_neighbors(size_t node_id) const;

    void for_each_out_edge(size_t node_id, std::function<void(size_t, size_t)> f) const;
    void for_each_in_edge(size_t node_id, std::function<void(size_t, size_t)> f) const;
    void for_each_edge(
        size_t node_id, std::function<void(size_t, size_t)> f
    ) const; // TODO aggiungere struct invece di pair non chiare??

    auto get_out_edges(size_t node_id) const;
    auto get_in_edges(size_t node_id) const;
    auto get_edges(size_t node_id) const;

    size_t get_out_degree_of_node(size_t node_id) const;
    size_t get_in_degree_of_node(size_t node_id) const;
    size_t get_degree_of_node(size_t node_id) const;

    bool has_node(size_t node_id) const;
    bool has_edge_id(size_t edge_id) const;
    bool has_edge(size_t from_id, size_t to_id) const;
    bool are_neighbors(size_t node_1_id, size_t node_2_id) const;
    Edge get_edge(size_t edge_id) const;

    size_t get_number_of_nodes() const;
    size_t get_number_of_edges() const;

    std::string to_string(bool undirected) const;
    std::string to_string() const;
    std::string to_string(
        bool undirected, const utilities::NodesLabels& labels, const std::string_view name
    ) const;
    void print(bool undirected) const;
};

inline auto Graph::get_node_ids() const {
    return std::views::iota(size_t{0}, get_number_of_nodes());
}

inline auto Graph::get_out_neighbors(size_t node_id) const {
    return std::views::transform(m_out_adjacency_list[node_id], [&](size_t edge_id) {
        return get_edge(edge_id).to_id;
    });
}

inline auto Graph::get_in_neighbors(size_t node_id) const {
    return std::views::transform(m_in_adjacency_list[node_id], [&](size_t edge_id) {
        return get_edge(edge_id).from_id;
    });
}

inline auto Graph::get_neighbors(size_t node_id) const {
    return std::views::concat(get_out_neighbors(node_id), get_in_neighbors(node_id));
}

inline auto Graph::get_out_edges(size_t node_id) const {
    return std::views::transform(m_out_adjacency_list[node_id], [&](size_t edge_id) {
        return std::make_pair(edge_id, get_edge(edge_id).to_id);
    });
}

inline auto Graph::get_in_edges(size_t node_id) const {
    return std::views::transform(m_in_adjacency_list[node_id], [&](size_t edge_id) {
        return std::make_pair(edge_id, get_edge(edge_id).from_id);
    });
}

inline auto Graph::get_edges(size_t node_id) const {
    return std::views::concat(get_out_edges(node_id), get_in_edges(node_id));
}

} // namespace domus::graph