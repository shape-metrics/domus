#include "domus/core/graph/graph_utilities.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph::utilities {
using namespace domus::graph;

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



EdgesContainer::EdgesContainer(size_t number_of_edges_ids)
    : m_has_edge(number_of_edges_ids, false) {}

void EdgesContainer::add_edge(size_t edge_id) {
    DOMUS_ASSERT(!has_edge(edge_id), "EdgesContainer::add_edge: edge already exists");
    m_has_edge[edge_id] = true;
    m_number_of_edges++;
}

bool EdgesContainer::has_edge(size_t edge_id) const { return m_has_edge.at(edge_id); }

size_t EdgesContainer::size() const { return m_number_of_edges; }

bool EdgesContainer::empty() const { return size() == 0; }

void EdgesContainer::erase(size_t edge_id) {
    DOMUS_ASSERT(has_edge(edge_id), "EdgesContainer::erase: edge does not exist");
    m_has_edge[edge_id] = false;
    m_number_of_edges--;
}

OrientedEdgesContainer::OrientedEdgesContainer(size_t number_of_edges_ids)
    : m_visited_edges_1(number_of_edges_ids), m_visited_edges_2(number_of_edges_ids) {}

bool OrientedEdgesContainer::has_edge(size_t from_id, size_t to_id, size_t edge_id) const {
    if (from_id < to_id)
        return m_visited_edges_1.has_edge(edge_id);
    else
        return m_visited_edges_2.has_edge(edge_id);
}

void OrientedEdgesContainer::add_edge(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.add_edge(edge_id);
    else
        m_visited_edges_2.add_edge(edge_id);
}

void OrientedEdgesContainer::erase(size_t from_id, size_t to_id, size_t edge_id) {
    if (from_id < to_id)
        m_visited_edges_1.erase(edge_id);
    else
        m_visited_edges_2.erase(edge_id);
}

size_t OrientedEdgesContainer::size() const {
    return m_visited_edges_1.size() + m_visited_edges_2.size();
}

bool OrientedEdgesContainer::empty() const { return size() == 0; }


} // namespace domus::graph::utilities