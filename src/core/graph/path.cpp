#include "domus/core/graph/path.hpp"

#include <algorithm>

#include "domus/core/graph/graph.hpp"
#include "domus/core/print.hpp"

#include "domus/core/debug.hpp"

namespace domus::graph {

size_t Path::get_first_node_id() const { return m_nodes_ids[0]; }

size_t Path::get_last_node_id() const { return m_last_node_id.value(); }

size_t Path::get_first_edge_id() const { return m_edges_ids[0]; }

size_t Path::get_last_edge_id() const { return m_edges_ids[number_of_edges() - 1]; }

bool Path::contains_node_id(size_t node_id) const {
    if (get_last_node_id() == node_id)
        return true;
    return (std::ranges::find(m_nodes_ids, node_id) != m_nodes_ids.end());
}

bool Path::contains_edge_id(size_t edge_id) const {
    return (std::ranges::find(m_edges_ids, edge_id) != m_edges_ids.end());
}

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

void Path::pop_front() {
    DOMUS_ASSERT(number_of_edges() > 0, "Path::pop_front: path is empty");
    m_nodes_ids.pop_front();
    m_edges_ids.pop_front();
    if (number_of_edges() == 0)
        m_last_node_id = std::nullopt;
}

void Path::pop_back() {
    DOMUS_ASSERT(number_of_edges() > 0, "Path::pop_back: path is empty");
    if (number_of_edges() == 1) {
        m_nodes_ids.clear();
        m_edges_ids.clear();
        m_last_node_id = std::nullopt;
    } else {
        m_last_node_id = m_nodes_ids.back();
        m_nodes_ids.pop_back();
        m_edges_ids.pop_back();
    }
}

size_t Path::number_of_edges() const { return m_nodes_ids.size(); }

size_t Path::number_of_nodes() const {
    if (number_of_edges() == 0)
        return 0;
    return number_of_edges() + 1;
}

size_t Path::get_node_id_at_position(size_t position) const {
    DOMUS_ASSERT(position <= number_of_edges(), "Path::node_id_at_position: out of range");
    if (position == number_of_edges())
        return get_last_node_id();
    return m_nodes_ids[position];
}

size_t Path::get_edge_id_at_position(size_t position) const {
    DOMUS_ASSERT(position < number_of_edges(), "Path::edge_id_at_position: out of range");
    return m_edges_ids[position];
}

std::string Path::to_string(bool print_edge_ids) const {
    if (number_of_edges() == 0)
        return "Empty Path\n";
    std::string result;
    auto out = std::back_inserter(result);
    domus::format_to(out, "Path:");
    if (print_edge_ids)
        for (size_t i = 0; i < number_of_edges(); ++i)
            std::format_to(out, " {} <{}>", m_nodes_ids[i], m_edges_ids[i]);
    else
        for (size_t i = 0; i < number_of_edges(); ++i)
            std::format_to(out, " {} -", m_nodes_ids[i]);
    std::format_to(out, " {}", m_last_node_id.value());
    return result;
}

void Path::print(bool print_edge_ids) const { domus::print("{}", to_string(print_edge_ids)); }

void Path::println(bool print_edge_ids) const { domus::println("{}", to_string(print_edge_ids)); }

bool Path::operator==(const Path& other) const {
    return (m_nodes_ids == other.m_nodes_ids) && (m_edges_ids == other.m_edges_ids) &&
           (m_last_node_id == other.m_last_node_id);
}

Path convert_path(
    const Path& path,
    const utilities::NodesLabels<size_t>& node_labels,
    const utilities::EdgesLabels<size_t>& edge_labels,
    const Graph& graph
) {
    Path labeled_path;
    for (size_t i = 0; i < path.number_of_edges(); ++i) {
        const size_t node_1 = path.get_node_id_at_position(i);
        const size_t edge = path.get_edge_id_at_position(i);
        const size_t labeled_node_id = node_labels.get_label(node_1);
        const size_t labeled_edge_id = edge_labels.get_label(edge);
        labeled_path.push_back(graph, labeled_node_id, labeled_edge_id);
    }
    return labeled_path;
}

} // namespace domus::graph