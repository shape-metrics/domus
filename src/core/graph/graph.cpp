#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <format>
#include <iterator>
#include <print>
#include <string>

#include "../domus_assert.hpp"

size_t Graph::get_one_node_id() const { return m_nodes_ids.get_one_node_id(); }

bool Graph::has_node(size_t node_id) const { return m_nodes_ids.has_node(node_id); }

void Graph::for_each_node(std::function<void(size_t)> f) const { m_nodes_ids.for_each(f); }

void Graph::for_each_out_neighbors(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_out_neighbors: node does not exist");
    m_out_adjacency_list.get_neighbors_of_node(node_id).for_each(f);
}

void Graph::for_each_in_neighbors(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_in_neighbors: node does not exist");
    m_in_adjacency_list.get_neighbors_of_node(node_id).for_each(f);
}

void Graph::for_each_neighbor(size_t node_id, std::function<void(size_t)> f) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::for_each_neighbor: node does not exist");
    for_each_in_neighbors(node_id, f);
    for_each_out_neighbors(node_id, f);
}

void Graph::add_node(size_t id) {
    DOMUS_ASSERT(!has_node(id), "Graph::add_node: node already exists");
    m_nodes_ids.add_node(id);
}

size_t Graph::add_node() {
    while (has_node(m_next_node_id))
        m_next_node_id++;
    add_node(m_next_node_id);
    return m_next_node_id++;
}

size_t Graph::get_out_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(
        has_node(node_id),
        std::format("Graph::get_out_degree_of_node {} node does not exist", node_id)

    );
    return m_out_adjacency_list.get_neighbors_of_node(node_id).size();
}

size_t Graph::get_in_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::get_in_degree_of_node: node does not exist");
    return m_in_adjacency_list.get_neighbors_of_node(node_id).size();
}

size_t Graph::get_degree_of_node(size_t node_id) const {
    DOMUS_ASSERT(has_node(node_id), "Graph::get_degree_of_node: node does not exist");
    return get_out_degree_of_node(node_id) + get_in_degree_of_node(node_id);
}

void Graph::add_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::add_edge: node does not exist");
    DOMUS_ASSERT(!has_edge(from_id, to_id), "Graph::add_edge: edge already exists");
    m_out_adjacency_list.add_edge(from_id, to_id);
    m_in_adjacency_list.add_edge(to_id, from_id);
    m_total_edges++;
}

bool Graph::has_edge(size_t from_id, size_t to_id) const {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::has_edge: node does not exist");
    return m_out_adjacency_list.has_edge(from_id, to_id);
}

bool Graph::are_neighbors(size_t node_1_id, size_t node_2_id) const {
    return has_edge(node_1_id, node_2_id) || has_edge(node_2_id, node_1_id);
}

size_t Graph::size() const { return m_nodes_ids.size(); }

size_t Graph::get_number_of_edges() const { return m_total_edges; }

void Graph::remove_edge(size_t from_id, size_t to_id) {
    DOMUS_ASSERT(has_node(from_id) && has_node(to_id), "Graph::remove_edge: node does not exist");
    DOMUS_ASSERT(has_edge(from_id, to_id), "Graph::remove_edge: edge does not exist");
    m_out_adjacency_list.erase_edge(from_id, to_id);
    m_in_adjacency_list.erase_edge(to_id, from_id);
    m_total_edges--;
}

std::string Graph::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Graph:\n");
    for_each_node([&](size_t node_id) {
        std::format_to(out, "{}: out[ ", node_id);
        for_each_out_neighbors(node_id, [&](size_t neighbor_id) {
            std::format_to(out, "{} ", neighbor_id);
        });
        std::format_to(out, "] in[ ");
        for_each_in_neighbors(node_id, [&](size_t neighbor_id) {
            std::format_to(out, "{} ", neighbor_id);
        });
        std::format_to(out, "]\n");
    });
    return result;
}

void Graph::print() const { std::print("{}", to_string()); }