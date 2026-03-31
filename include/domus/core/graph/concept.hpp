#pragma once

#include <concepts>
#include <functional>
#include <utility>

namespace domus::graph {

struct Edge {
    size_t from_id;
    size_t to_id;
    bool operator==(const Edge& other) const {
        return from_id == other.from_id && to_id == other.to_id;
    }
};

struct EdgeIter {
    size_t id;
    size_t neighbor_id;
};

template <typename T>
concept UndirectedGraphLike = requires(const T a) {
    { a.get_nodes_ids() } -> std::ranges::view;

    { a.get_neighbors(std::declval<size_t>()) } -> std::ranges::view;
    { a.get_edges(std::declval<size_t>()) } -> std::ranges::view;

    // { a.get_all_edges() } -> std::ranges::view;

    { a.get_degree_of_node(std::declval<size_t>()) } -> std::same_as<size_t>;

    { a.has_node(std::declval<size_t>()) } -> std::same_as<bool>;
    // { a.has_edge_id(std::declval<size_t>()) } -> std::same_as<bool>;
    { a.are_neighbors(std::declval<size_t>(), std::declval<size_t>()) } -> std::same_as<bool>;
    // { a.get_edge(std::declval<size_t>()) } -> std::same_as<Edge>;

    { a.get_number_of_nodes() } -> std::same_as<size_t>;
    { a.get_number_of_edges() } -> std::same_as<size_t>;
} && requires(const T g, size_t u) {
    requires std::same_as<std::ranges::range_value_t<decltype(g.get_edges(u))>, EdgeIter>;

    requires std::same_as<std::ranges::range_value_t<decltype(g.get_neighbors(u))>, size_t>;
};

} // namespace domus::graph