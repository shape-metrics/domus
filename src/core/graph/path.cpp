#include "domus/core/graph/path.hpp"

#include <algorithm>
#include <print>

#include "domus/core/graph/graph.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph {

size_t Path::get_first_node_id() const { return m_nodes_ids[0]; }

size_t Path::get_last_node_id() const { return m_last_node_id.value(); }

void Path::push_front(const Graph& graph, size_t next_node_id, size_t edge_id) {
    DOMUS_ASSERT(graph.has_node(next_node_id), "Path::insert: node does not exist");
    DOMUS_ASSERT(graph.has_edge_id(edge_id), "Path::insert: edge does not exist");
    DOMUS_ASSERT(
        number_of_edges() == 0 || next_node_id == get_first_node_id(),
        "Path::insert: next node is not the first"
    );
    auto [from_id, to_id] = graph.get_edge(edge_id);
    if (from_id == next_node_id)
        std::swap(from_id, to_id);
    DOMUS_ASSERT(next_node_id == to_id, "Path::insert: next_node_id is not to_id");
    if (number_of_edges() == 0)
        m_last_node_id = to_id;
    m_nodes_ids.push_front(from_id);
    m_edges_ids.push_front(edge_id);
}

void Path::push_back(const Graph& graph, size_t prev_node_id, size_t edge_id) {
    DOMUS_ASSERT(graph.has_node(prev_node_id), "Path::append: node does not exist");
    DOMUS_ASSERT(graph.has_edge_id(edge_id), "Path::append: edge does not exist");
    DOMUS_ASSERT(
        number_of_edges() == 0 || prev_node_id == get_last_node_id(),
        "Path::append: prev_node_id is not the last"
    );
    auto [from_id, to_id] = graph.get_edge(edge_id);
    if (to_id == prev_node_id)
        std::swap(from_id, to_id);
    DOMUS_ASSERT(prev_node_id == from_id, "Path::append: prev_node_id is not from_id");
    m_last_node_id = to_id;
    m_nodes_ids.push_back(prev_node_id);
    m_edges_ids.push_back(edge_id);
}

void Path::reverse() {
    const size_t first = get_first_node_id();
    const size_t last = get_last_node_id();
    std::ranges::reverse(m_nodes_ids);
    std::ranges::reverse(m_edges_ids);
    m_nodes_ids.push_front(last);
    m_nodes_ids.pop_back();
    m_last_node_id = first;
}

size_t Path::number_of_edges() const { return m_nodes_ids.size(); }

size_t Path::node_id_at_position(size_t position) const {
    DOMUS_ASSERT(position <= number_of_edges(), "Path::node_id_at_position: out of range");
    if (position == number_of_edges())
        return get_last_node_id();
    return m_nodes_ids[position];
}

size_t Path::edge_id_at_position(size_t position) const {
    DOMUS_ASSERT(position < number_of_edges(), "Path::edge_id_at_position: out of range");
    return m_edges_ids[position];
}

std::string Path::to_string() const {
    if (number_of_edges() == 0)
        return "Empty Path\n";
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Path:");
    for (size_t i = 0; i < number_of_edges(); ++i) {
        std::format_to(out, " {} <{}>", m_nodes_ids[i], m_edges_ids[i]);
    }
    std::format_to(out, " {}\n", m_last_node_id.value());
    return result;
}

void Path::print() const { std::print("{}", to_string()); }

} // namespace domus::graph