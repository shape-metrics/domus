#include "domus/core/graph/graph.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <print>
#include <string>

#include "domus/core/graph/graph_utilities.hpp"

#include "../domus_debug.hpp"

using namespace domus::graph;

bool Graph::has_node(size_t node_id) const { return node_id < get_number_of_nodes(); }

void Graph::for_each_node(std::function<void(size_t)> f) const {
    for (size_t node_id = 0; node_id < get_number_of_nodes(); ++node_id) {
        f(node_id);
    }
}

void Graph::for_each_out_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_out_neighbors: node does not exist");
    for (const size_t edge_id : m_out_adjacency_list[node_id])
        f(m_edges[edge_id]->edge.to_id);
}

void Graph::for_each_in_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_in_neighbors: node does not exist");
    for (const size_t edge_id : m_in_adjacency_list[node_id])
        f(m_edges[edge_id]->edge.from_id);
}

void Graph::for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_neighbor: node does not exist");
    for_each_in_neighbor(node_id, f);
    for_each_out_neighbor(node_id, f);
}

void Graph::for_each_out_edge(size_t node_id, std::function<void(EdgeIter)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_out_edge: node does not exist");
    for (const size_t edge_id : m_out_adjacency_list[node_id])
        f(EdgeIter{edge_id, m_edges[edge_id]->edge.to_id});
}

void Graph::for_each_in_edge(size_t node_id, std::function<void(EdgeIter)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_in_edge: node does not exist");
    for (const size_t edge_id : m_in_adjacency_list[node_id])
        f(EdgeIter{edge_id, m_edges[edge_id]->edge.from_id});
}

void Graph::for_each_edge(size_t node_id, std::function<void(EdgeIter)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_edge: node does not exist");
    for_each_in_edge(node_id, f);
    for_each_out_edge(node_id, f);
}

size_t Graph::add_node() {
    m_in_adjacency_list.push_back({});
    m_out_adjacency_list.push_back({});
    return m_in_adjacency_list.size() - 1;
}

size_t Graph::get_out_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(
        has_node(node_id),
        "Graph::get_out_degree_of_node {} node does not exist",
        node_id
    );
    return m_out_adjacency_list[node_id].size();
}

size_t Graph::get_in_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::get_in_degree_of_node: node does not exist");
    return m_in_adjacency_list[node_id].size();
}

size_t Graph::get_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::get_degree_of_node: node does not exist");
    return get_out_degree_of_node(node_id) + get_in_degree_of_node(node_id);
}

size_t Graph::add_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::add_edge: node does not exist");
    DOMUS_ASSERT(!has_edge(from_id, to_id), "Graph::add_edge: edge already exists");
    DOMUS_ASSERT(from_id != to_id, "graph::add_edge: from and to are equal");
    size_t edge_id;
    if (m_free_edges_ids.empty()) {
        edge_id = m_edges.size();
        m_edges.emplace_back(EdgeId{edge_id, Edge{from_id, to_id}});
    } else {
        edge_id = m_free_edges_ids.top();
        m_free_edges_ids.pop();
        m_edges[edge_id] = EdgeId{edge_id, Edge{from_id, to_id}};
    }
    m_out_adjacency_list[from_id].push_back(edge_id);
    m_in_adjacency_list[to_id].push_back(edge_id);
    return edge_id;
}

bool Graph::has_edge(size_t from_id, size_t to_id) const {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::has_edge: node does not exist");
    return std::ranges::any_of(m_out_adjacency_list[from_id], [&](const size_t edge_id) {
        return m_edges[edge_id]->edge.to_id == to_id;
    });
}

bool Graph::are_neighbors(size_t node_1_id, size_t node_2_id) const {
    return has_edge(node_1_id, node_2_id) || has_edge(node_2_id, node_1_id);
}

Edge Graph::get_edge(size_t edge_id) const {
    DOMUS_ASSERT(has_edge_id(edge_id), "Graph::get_edge: edge does not exist");
    return m_edges[edge_id]->edge;
}

size_t Graph::get_number_of_nodes() const { return m_in_adjacency_list.size(); }

size_t Graph::get_number_of_edges() const { return m_edges.size() - m_free_edges_ids.size(); }

size_t Graph::remove_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(from_id != to_id, "Graph::remove_edge: from_id and to_id are equal");
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::remove_edge: node does not exist");
    auto it_1 = std::ranges::find_if(m_out_adjacency_list[from_id], [&](const size_t edge_id) {
        return m_edges[edge_id]->edge.to_id == to_id;
    });
    auto it_2 = std::ranges::find_if(m_in_adjacency_list[to_id], [&](const size_t edge_id) {
        return m_edges[edge_id]->edge.from_id == from_id;
    });
    DOMUS_ASSERT(it_1 != m_out_adjacency_list[from_id].end(), "Graph::remove_edge: edge not found");
    DOMUS_ASSERT(it_2 != m_in_adjacency_list[to_id].end(), "Graph::remove_edge: edge not found");
    DOMUS_ASSERT(*it_1 == *it_2, "Graph::remove_edge: edge_ids do not coincide");

    const size_t edge_id = *it_1;

    *it_1 = m_out_adjacency_list[from_id].back();
    m_out_adjacency_list[from_id].pop_back();

    *it_2 = m_in_adjacency_list[to_id].back();
    m_in_adjacency_list[to_id].pop_back();

    m_free_edges_ids.push(edge_id);
    m_edges[edge_id] = std::nullopt;

    return edge_id;
}

Subdivision Graph::subdivide_edge(size_t edge_id) {
    DOMUS_ASSERT(has_edge_id(edge_id), "Graph::subdivide_edge: edge does not exist");
    const auto [from_id, to_id] = m_edges[edge_id]->edge;
    remove_edge(edge_id);
    const size_t in_between_id = add_node();

    const size_t edge_from_between_id = add_edge(from_id, in_between_id);
    const size_t edge_between_to_id = add_edge(in_between_id, to_id);
    return {from_id, in_between_id, to_id, edge_from_between_id, edge_between_to_id};
}

void Graph::remove_edge(size_t edge_id) {
    DOMUS_ASSERT(has_edge_id(edge_id), "Graph::remove_edge: edge does not exist");
    auto [from_id, to_id] = m_edges[edge_id]->edge;
    remove_edge(from_id, to_id);
}

bool Graph::has_edge_id(size_t edge_id) const {
    return edge_id < m_edges.size() && m_edges[edge_id].has_value();
}

std::string Graph::to_string() const { return to_string(true); }

std::string Graph::to_string(bool undirected) const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Graph:\n");
    for_each_node([&](size_t node_id) {
        if (undirected) {
            std::format_to(out, "{}: [ ", node_id);
            for_each_neighbor(node_id, [&](size_t neighbor_id) {
                std::format_to(out, "{} ", neighbor_id);
            });
            std::format_to(out, "]\n");
        } else {
            std::format_to(out, "{}: out[ ", node_id);
            for_each_out_neighbor(node_id, [&](size_t neighbor_id) {
                std::format_to(out, "{} ", neighbor_id);
            });
            std::format_to(out, "] in[ ");
            for_each_in_neighbor(node_id, [&](size_t neighbor_id) {
                std::format_to(out, "{} ", neighbor_id);
            });
            std::format_to(out, "]\n");
        }
    });
    return result;
}

void Graph::print(bool undirected) const { std::print("{}", to_string(undirected)); }

std::string Graph::to_string(
    bool undirected, const utilities::NodesLabels& labels, const std::string_view name
) const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "{}:\n", name);
    for_each_node([&](const size_t node_id) {
        const size_t node_label = labels.get_label(node_id);
        if (undirected) {
            std::format_to(out, "{}: [ ", node_label);
            for_each_neighbor(node_id, [&](size_t neighbor_id) {
                const size_t neighbor_label = labels.get_label(neighbor_id);
                std::format_to(out, "{} ", neighbor_label);
            });
            std::format_to(out, "]\n");
        } else {
            std::format_to(out, "{}: out[ ", node_label);
            for_each_out_neighbor(node_id, [&](size_t neighbor_id) {
                const size_t neighbor_label = labels.get_label(neighbor_id);
                std::format_to(out, "{} ", neighbor_label);
            });
            std::format_to(out, "] in[ ");
            for_each_in_neighbor(node_id, [&](size_t neighbor_id) {
                const size_t neighbor_label = labels.get_label(neighbor_id);
                std::format_to(out, "{} ", neighbor_label);
            });
            std::format_to(out, "]\n");
        }
    });
    return result;
}

bool Graph::add_subdivision_to_cycle(const Subdivision& subdivision, Cycle& cycle) const {
    DOMUS_ASSERT(
        has_node(subdivision.from_id) && has_node(subdivision.to_id) &&
            has_node(subdivision.in_between_id),
        "Graph::add_subdivision_to_cycle: node does not exist"
    );
    DOMUS_ASSERT(
        has_edge_id(subdivision.edge_between_to_id) &&
            has_edge_id(subdivision.edge_from_between_id),
        "Graph::add_subdivision_to_cycle: edge does not exist"
    );
    const size_t from_node_id = subdivision.from_id;
    const size_t to_node_id = subdivision.to_id;
    const size_t in_between_node_id = subdivision.in_between_id;

    for (size_t i = 0; i < cycle.size(); ++i) {
        const size_t node_id = cycle.node_id_at(i);
        const size_t next_id = cycle.node_id_at(i + 1);

        if (node_id == from_node_id && next_id == to_node_id) {
            cycle.m_edges_ids[i] = subdivision.edge_from_between_id;
            cycle.m_edges_ids.insert(
                cycle.m_edges_ids.begin() + static_cast<long>(i + 1),
                subdivision.edge_between_to_id
            );
            cycle.m_nodes_ids.insert(
                cycle.m_nodes_ids.begin() + static_cast<long>(i + 1),
                in_between_node_id
            );
            return true;
        } else if (node_id == to_node_id && next_id == from_node_id) {
            cycle.m_edges_ids[i] = subdivision.edge_between_to_id;
            cycle.m_edges_ids.insert(
                cycle.m_edges_ids.begin() + static_cast<long>(i + 1),
                subdivision.edge_from_between_id
            );
            cycle.m_nodes_ids.insert(
                cycle.m_nodes_ids.begin() + static_cast<long>(i + 1),
                in_between_node_id
            );
            return true;
        }
    }
    return false;
}