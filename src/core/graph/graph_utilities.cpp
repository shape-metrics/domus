#include "domus/core/graph/graph_utilities.hpp"

#include <algorithm>

#include "../domus_debug.hpp"
#include "domus/core/graph/graph.hpp"

using namespace domus::graph::utilities;
using namespace domus::graph;

NodesContainer::NodesContainer(const Graph& graph)
    : m_has_node(graph.get_number_of_nodes(), false) {}

void NodesContainer::add_node(size_t node_id) {
    DOMUS_ASSERT(!has_node(node_id), "NodesContainer::add_node: node already exists");
    m_has_node[node_id] = true;
    m_number_of_nodes++;
}

bool NodesContainer::has_node(size_t node_id) const { return m_has_node.at(node_id); }

size_t NodesContainer::size() const { return m_number_of_nodes; }

bool NodesContainer::empty() const { return size() == 0; }

void NodesContainer::erase(size_t node_id) {
    DOMUS_ASSERT(has_node(node_id), "NodesContainer::erase: node does not exist");
    m_has_node[node_id] = false;
    m_number_of_nodes--;
}

NodesLabels::NodesLabels(const Graph& graph) { m_labels.resize(graph.get_number_of_nodes()); }

void NodesLabels::add_label(size_t node_id, size_t label) {
    DOMUS_ASSERT(!has_label(node_id), "NodesLabels::add_label: node already has a label");
    m_labels[node_id] = label;
}

bool NodesLabels::has_label(size_t node_id) const { return m_labels[node_id].has_value(); }

size_t NodesLabels::get_label(size_t node_id) const {
    DOMUS_ASSERT(
        has_label(node_id),
        "NodesLabels::get_label: node {} does not have a label",
        node_id
    );
    return m_labels[node_id].value();
}

void NodesLabels::erase_label(size_t node_id) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::erase_label: node does not have a label");
    m_labels[node_id].reset();
}

void NodesLabels::update_label(size_t node_id, size_t new_label) {
    DOMUS_ASSERT(has_label(node_id), "NodesLabels::update_label: node does not have a label");
    m_labels[node_id] = new_label;
}

size_t GraphPath::get_first_node_id() const { return m_nodes_ids[0]; }

size_t GraphPath::get_last_node_id() const { return m_last_node_id.value(); }

void GraphPath::push_front(const Graph& graph, size_t next_node_id, size_t edge_id) {
    DOMUS_ASSERT(graph.has_node(next_node_id), "GraphPath::insert: node does not exist");
    DOMUS_ASSERT(graph.has_edge_id(edge_id), "GraphPath::insert: edge does not exist");
    DOMUS_ASSERT(
        number_of_edges() == 0 || next_node_id == get_first_node_id(),
        "GraphPath::insert: next node is not the first"
    );
    auto [from_id, to_id] = graph.get_edge(edge_id);
    if (from_id == next_node_id)
        std::swap(from_id, to_id);
    DOMUS_ASSERT(next_node_id == to_id, "GraphPath::insert: next_node_id is not to_id");
    if (number_of_edges() == 0)
        m_last_node_id = to_id;
    m_nodes_ids.push_front(from_id);
    m_edges_ids.push_front(edge_id);
}

void GraphPath::push_back(const Graph& graph, size_t prev_node_id, size_t edge_id) {
    DOMUS_ASSERT(graph.has_node(prev_node_id), "GraphPath::append: node does not exist");
    DOMUS_ASSERT(graph.has_edge_id(edge_id), "GraphPath::append: edge does not exist");
    DOMUS_ASSERT(
        number_of_edges() == 0 || prev_node_id == get_last_node_id(),
        "GraphPath::append: prev_node_id is not the last"
    );
    auto [from_id, to_id] = graph.get_edge(edge_id);
    if (to_id == prev_node_id)
        std::swap(from_id, to_id);
    DOMUS_ASSERT(prev_node_id == from_id, "GraphPath::append: prev_node_id is not from_id");
    m_last_node_id = to_id;
    m_nodes_ids.push_back(prev_node_id);
    m_edges_ids.push_back(edge_id);
}

void GraphPath::reverse() {
    const size_t first = get_first_node_id();
    const size_t last = get_last_node_id();
    std::ranges::reverse(m_nodes_ids);
    std::ranges::reverse(m_edges_ids);
    m_nodes_ids.push_front(last);
    m_nodes_ids.pop_back();
    m_last_node_id = first;
}

void GraphPath::for_each(std::function<void(size_t, size_t)> f) const { // edge_id, prev_node_id
    for (size_t i = 0; i < number_of_edges(); ++i)
        f(m_edges_ids[i], m_nodes_ids[i]);
}

size_t GraphPath::number_of_edges() const { return m_nodes_ids.size(); }

std::string GraphPath::to_string() const {
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

void GraphPath::print() const { std::print("{}", to_string()); }

EdgesLabels::EdgesLabels(const Graph& graph) { m_labels.resize(graph.get_number_of_edges()); }

EdgesLabels::EdgesLabels(size_t number_of_edges) { m_labels.resize(number_of_edges); }

void EdgesLabels::add_label(size_t edge_id, size_t label) {
    DOMUS_ASSERT(!has_label(edge_id), "EdgesLabels::add_label: edge already has a label");
    m_labels[edge_id] = label;
}

bool EdgesLabels::has_label(size_t edge_id) const { return m_labels[edge_id].has_value(); }

size_t EdgesLabels::get_label(size_t edge_id) const {
    DOMUS_ASSERT(
        has_label(edge_id),
        "EdgesLabels::get_label: edge {} does not have a label",
        edge_id
    );
    return *m_labels[edge_id];
}

void EdgesLabels::erase_label(size_t edge_id) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::erase_label: edge does not have a label");
    m_labels[edge_id].reset();
}

void EdgesLabels::update_label(size_t edge_id, size_t new_label) {
    DOMUS_ASSERT(has_label(edge_id), "EdgesLabels::update_label: edge does not have a label");
    m_labels[edge_id] = new_label;
}

void EdgesLabels::update_size(size_t edge_id) {
    while (m_labels.size() <= edge_id)
        m_labels.push_back(std::nullopt);
}