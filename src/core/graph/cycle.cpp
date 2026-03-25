
#include "domus/core/graph/cycle.hpp"

#include <algorithm>
#include <format>
#include <print>

#include "domus/core/graph/path.hpp"

#include "../domus_debug.hpp"

namespace domus::graph {

Cycle::Cycle(const Path& path) {
    DOMUS_ASSERT(
        path.get_first_node_id() == path.get_last_node_id(),
        "Cycle::Cycle: path is not a cycle"
    );
    path.for_each([this](size_t edge_id, size_t prev_node_id) {
        m_nodes_ids.push_back(prev_node_id);
        m_edges_ids.push_back(edge_id);
    });
}

bool Cycle::empty() const { return m_nodes_ids.empty(); }

size_t Cycle::size() const { return m_nodes_ids.size(); }

size_t Cycle::node_id_at(size_t index) const { return m_nodes_ids[index % size()]; }

std::string Cycle::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Cycle: [ ");
    for (size_t i = 0; i < size(); ++i) {
        size_t node_id = node_id_at(i);
        size_t edge_id = edge_id_at(i);
        std::format_to(out, "{} <{}> ", node_id, edge_id);
    }
    std::format_to(out, "{} ]\n", node_id_at(0));
    return result;
}

void Cycle::print() const { std::print("{}", to_string()); }

bool Cycle::has_node_id(size_t node_id) const {
    return std::ranges::find(m_nodes_ids, node_id) != m_nodes_ids.end();
}

size_t Cycle::node_id_position(size_t node_id) const {
    auto it = std::ranges::find(m_nodes_ids, node_id);
    DOMUS_ASSERT(it != m_nodes_ids.end(), "Cycle::node_position: node {} is not in cycle", node_id);
    return static_cast<size_t>(std::distance(m_nodes_ids.begin(), it));
}

void Cycle::for_each(std::function<void(size_t)> func) const {
    for (size_t i = 0; i < size(); ++i)
        func(node_id_at(i));
}

size_t Cycle::edge_id_at(size_t index) const { return m_edges_ids[index % size()]; }

bool Cycle::has_edge_id(size_t edge_id) const {
    return std::ranges::contains(m_edges_ids, edge_id);
}

} // namespace domus::graph