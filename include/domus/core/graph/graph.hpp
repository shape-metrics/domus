#pragma once

#include <ranges>
#include <stack>
#include <string>
#include <vector>

#include "domus/core/graph/concept.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::graph {

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

    auto get_nodes_ids() const;

    auto get_out_neighbors(size_t node_id) const;
    auto get_in_neighbors(size_t node_id) const;
    auto get_neighbors(size_t node_id) const;

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

    auto get_all_edges() const;

    size_t get_number_of_nodes() const;
    size_t get_number_of_edges() const;

    std::string to_string(bool undirected) const;
    std::string to_string() const;
    std::string to_string(
        bool undirected, const utilities::NodesLabels& labels, const std::string_view name
    ) const;
    void print(bool undirected) const;
};

inline auto Graph::get_nodes_ids() const {
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
        return EdgeIter{edge_id, get_edge(edge_id).to_id};
    });
}

inline auto Graph::get_in_edges(size_t node_id) const {
    return std::views::transform(m_in_adjacency_list[node_id], [&](size_t edge_id) {
        return EdgeIter{edge_id, get_edge(edge_id).from_id};
    });
}

inline auto Graph::get_edges(size_t node_id) const {
    return std::views::concat(get_out_edges(node_id), get_in_edges(node_id));
}

inline auto Graph::get_all_edges() const {
    return m_edges |
           std::views::filter([](const std::optional<EdgeId>& edge) { return edge.has_value(); }) |
           std::views::transform([](const std::optional<EdgeId>& edge) { return *edge; });
}

} // namespace domus::graph