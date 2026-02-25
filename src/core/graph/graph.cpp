#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <cassert>
#include <print>

using namespace std;

bool DirectedGraph::has_node(int node_id) const { return m_nodes_ids.has_node(node_id); }

const NodesContainer& DirectedGraph::get_nodes_ids() const { return m_nodes_ids; }

const NodesContainer& DirectedGraph::get_out_neighbors_of_node(int node_id) const {
    assert(has_node(node_id) && "Node does not exist");
    return m_out_adjacency_list.get_neighbors_of_node(node_id);
}

const NodesContainer& DirectedGraph::get_in_neighbors_of_node(int node_id) const {
    assert(has_node(node_id) && "Node does not exist");
    return m_in_adjacency_list.get_neighbors_of_node(node_id);
}

void DirectedGraph::add_node(int id) {
    assert(!has_node(id) && "Node already exists");
    m_nodes_ids.add_node(id);
}

int DirectedGraph::add_node() {
    while (has_node(m_next_node_id))
        m_next_node_id++;
    add_node(m_next_node_id);
    return m_next_node_id++;
}

size_t DirectedGraph::get_out_degree_of_node(int node_id) const {
    return get_out_neighbors_of_node(node_id).size();
}

size_t DirectedGraph::get_in_degree_of_node(int node_id) const {
    return get_in_neighbors_of_node(node_id).size();
}

void DirectedGraph::add_edge(int from_id, int to_id) {
    assert(has_node(from_id) && has_node(to_id) && "Node does not exist");
    assert(!has_edge(from_id, to_id) && "Edge already exists");
    m_out_adjacency_list.add_edge(from_id, to_id);
    m_in_adjacency_list.add_edge(to_id, from_id);
    m_total_edges++;
}

bool DirectedGraph::has_edge(int from_id, int to_id) const {
    assert(has_node(from_id) && has_node(to_id) && "Node does not exist");
    return m_out_adjacency_list.has_edge(from_id, to_id);
}

size_t DirectedGraph::size() const { return m_nodes_ids.size(); }
size_t DirectedGraph::get_number_of_edges() const { return m_total_edges; }

void DirectedGraph::remove_node(int node_id) {
    assert(has_node(node_id) && "Node does not exist");
    m_nodes_ids.erase(node_id);
    m_total_edges -= get_out_degree_of_node(node_id);
    m_total_edges -= get_in_degree_of_node(node_id);
    get_out_neighbors_of_node(node_id).for_each([this, node_id](int neighbor_id) {
        m_in_adjacency_list.erase_edge(neighbor_id, node_id);
    });
    get_in_neighbors_of_node(node_id).for_each([this, node_id](int neighbor_id) {
        m_out_adjacency_list.erase_edge(neighbor_id, node_id);
    });
}

void DirectedGraph::remove_edge(int from_id, int to_id) {
    assert(has_node(from_id) && has_node(to_id) && "Node does not exist");
    assert(has_edge(from_id, to_id) && "Edge does not exist");
    m_out_adjacency_list.erase_edge(from_id, to_id);
    m_in_adjacency_list.erase_edge(to_id, from_id);
    m_total_edges--;
}
string DirectedGraph::to_string() const {
    string result = "DirectedGraph:\n";
    get_nodes_ids().for_each([&result, this](int node_id) {
        result += std::to_string(node_id) + ": ";
        get_out_neighbors_of_node(node_id).for_each([&result](int neighbor_id) {
            result += std::to_string(neighbor_id) + " ";
        });
        result += "\n";
    });
    return result;
}

void DirectedGraph::print() const { std::println("{}", to_string()); }

bool UndirectedGraph::has_node(int node_id) const { return m_nodes_ids.has_node(node_id); }

const NodesContainer& UndirectedGraph::get_nodes_ids() const { return m_nodes_ids; }

const NodesContainer& UndirectedGraph::get_neighbors_of_node(int node_id) const {
    return m_adjacency_list.get_neighbors_of_node(node_id);
}

void UndirectedGraph::add_node(int id) {
    assert(!has_node(id) && "Node already exists");
    m_nodes_ids.add_node(id);
}

int UndirectedGraph::add_node() {
    while (has_node(m_next_node_id))
        m_next_node_id++;
    add_node(m_next_node_id);
    return m_next_node_id++;
}

size_t UndirectedGraph::get_degree_of_node(int node_id) const {
    return get_neighbors_of_node(node_id).size();
}

void UndirectedGraph::add_edge(int node_1_id, int node_2_id) {
    assert(has_node(node_1_id) && has_node(node_2_id) && "Node does not exist");
    assert(!has_edge(node_1_id, node_2_id) && "Edge already exists");
    m_adjacency_list.add_edge(node_1_id, node_2_id);
    m_adjacency_list.add_edge(node_2_id, node_1_id);
    m_total_edges++;
}

bool UndirectedGraph::has_edge(int node_1_id, int node_2_id) const {
    return m_adjacency_list.has_edge(node_1_id, node_2_id);
}

size_t UndirectedGraph::size() const { return m_nodes_ids.size(); }

size_t UndirectedGraph::get_number_of_edges() const { return m_total_edges; }

void UndirectedGraph::remove_node(int node_id) {
    assert(has_node(node_id) && "Node does not exist");
    m_total_edges -= get_degree_of_node(node_id);
    get_neighbors_of_node(node_id).for_each([node_id, this](int neighbor_id) {
        m_adjacency_list.erase_edge(neighbor_id, node_id);
    });
    m_adjacency_list.erase_node(node_id);
    m_nodes_ids.erase(node_id);
}

void UndirectedGraph::remove_edge(int node_1_id, int node_2_id) {
    assert(has_node(node_1_id) && has_node(node_2_id) && "Node does not exist");
    assert(has_edge(node_1_id, node_2_id) && "Edge does not exist");
    m_adjacency_list.erase_edge(node_1_id, node_2_id);
    m_adjacency_list.erase_edge(node_2_id, node_1_id);
    m_total_edges--;
}

string UndirectedGraph::to_string() const {
    string result = "UndirectedGraph:\n";
    get_nodes_ids().for_each([&result, this](int node_id) {
        result += std::to_string(node_id) + ": ";
        get_neighbors_of_node(node_id).for_each([&result](int neighbor_id) {
            result += std::to_string(neighbor_id) + " ";
        });
        result += "\n";
    });
    return result;
}

void UndirectedGraph::print() const { std::println("{}", to_string()); }