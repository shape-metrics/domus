#include "domus/core/graph/graph.hpp"

#include <format>
#include <iterator>
#include <print>
#include <string>

#include "domus/core/graph/graph_utilities.hpp"

#include "../domus_debug.hpp"

bool Graph::has_node(size_t node_id) const { return node_id < size(); }

void Graph::for_each_node(std::function<void(size_t)> f) const {
    for (size_t node_id = 0; node_id < size(); ++node_id) {
        f(node_id);
    }
}

void Graph::for_each_out_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_out_neighbors: node does not exist");
    for (size_t neighbor_id : m_out_adjacency_list[node_id])
        f(neighbor_id);
}

void Graph::for_each_in_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_in_neighbors: node does not exist");
    for (size_t neighbor_id : m_in_adjacency_list[node_id])
        f(neighbor_id);
}

void Graph::for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_neighbor: node does not exist");
    for_each_in_neighbor(node_id, f);
    for_each_out_neighbor(node_id, f);
}

size_t Graph::add_node() {
    m_in_adjacency_list.push_back({});
    m_out_adjacency_list.push_back({});
    return m_total_nodes++;
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

void Graph::add_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::add_edge: node does not exist");
    DOMUS_ASSERT(!has_edge(from_id, to_id), "Graph::add_edge: edge already exists");
    m_out_adjacency_list[from_id].push_back(to_id);
    m_in_adjacency_list[to_id].push_back(from_id);
    m_total_edges++;
}

bool Graph::has_edge(size_t from_id, size_t to_id) const {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::has_edge: node does not exist");
    return std::find(
               m_out_adjacency_list[from_id].begin(),
               m_out_adjacency_list[from_id].end(),
               to_id
           ) != m_out_adjacency_list[from_id].end();
}

bool Graph::are_neighbors(size_t node_1_id, size_t node_2_id) const {
    return has_edge(node_1_id, node_2_id) || has_edge(node_2_id, node_1_id);
}

size_t Graph::size() const { return m_total_nodes; }

size_t Graph::get_number_of_edges() const { return m_total_edges; }

void Graph::remove_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::remove_edge: node does not exist");
    DOMUS_ASSERT(has_edge(from_id, to_id), "Graph::remove_edge: edge does not exist");

    *std::find(m_out_adjacency_list[from_id].begin(), m_out_adjacency_list[from_id].end(), to_id) =
        m_out_adjacency_list[from_id].back();
    m_out_adjacency_list[from_id].pop_back();

    *std::find(m_in_adjacency_list[to_id].begin(), m_in_adjacency_list[to_id].end(), from_id) =
        m_in_adjacency_list[to_id].back();
    m_in_adjacency_list[to_id].pop_back();

    m_total_edges--;
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

std::string
Graph::to_string(bool undirected, const NodesLabels& labels, const std::string_view name) const {
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