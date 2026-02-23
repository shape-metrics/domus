#include "domus/planarity/embedding.hpp"

#include <cassert>
#include <functional>
#include <iostream>
#include <stack>
#include <unordered_set>
#include <utility>

using namespace std;

Embedding::Embedding(const UndirectedGraph& graph) {
    for (const int node_id : graph.get_nodes_ids())
        adjacency_list[node_id];
}

void Embedding::add_edge(const int from_id, const int to_id) {
    assert(!m_edges.contains({from_id, to_id}) && "Edge already exists");
    m_edges.insert({from_id, to_id});
    if (m_edges_to_add.contains({from_id, to_id})) {
        m_edges_to_add.erase({from_id, to_id});
    } else {
        m_edges_to_add.insert({to_id, from_id});
    }
    adjacency_list.at(from_id).append(to_id);
    number_of_edges_m++;
}

bool Embedding::is_consistent() const { return m_edges_to_add.empty(); }

const CircularSequence<int>& Embedding::get_adjacency_list(const int node_id) const {
    return adjacency_list.at(node_id);
}

string Embedding::to_string() const {
    string result;
    for (const auto& [node_id, neighbors] : adjacency_list) {
        result += "Node " + std::to_string(node_id) + " neighbors:";
        for (const int neighbor_id : neighbors)
            result += " " + std::to_string(neighbor_id);
        result += "\n";
    }
    return result;
}

size_t Embedding::size() const { return adjacency_list.size(); }

size_t Embedding::total_number_of_edges() const { return number_of_edges_m; }

void Embedding::print() const { std::cout << to_string(); }

size_t compute_number_of_faces_in_embedding(const Embedding& embedding) {
    size_t number_of_faces = 0;
    GraphEdgeHashSet visited_edges; // visited oriented edges
    for (int node_id : embedding.get_nodes_ids()) {
        for (int neighbor_id : embedding.get_adjacency_list(node_id)) {
            pair oriented_edge = {node_id, neighbor_id};
            if (visited_edges.contains(oriented_edge))
                continue;
            ++number_of_faces;
            int current_node = node_id;
            int next_node = neighbor_id;
            visited_edges.insert(oriented_edge);
            while (true) {
                int successor = embedding.get_adjacency_list(next_node).next_element(current_node);
                pair next_oriented_edge = {next_node, successor};
                if (visited_edges.contains(next_oriented_edge))
                    break;
                visited_edges.insert(next_oriented_edge);
                current_node = next_node;
                next_node = successor;
                if (current_node == node_id && next_node == neighbor_id)
                    break;
            }
        }
    }
    return number_of_faces;
}

bool is_embedding_planar(const Embedding& embedding) {
    return compute_embedding_genus(embedding) == 0;
}

size_t compute_number_of_connected_components(const Embedding& embedding) {
    assert(embedding.is_consistent() && "Embedding is not fully undirected");
    unordered_set<int> visited;
    size_t components = 0;
    const function<void(int)> explore_component = [&](const int start_node_id) {
        stack<int> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            const int node_id = stack.top();
            stack.pop();
            if (visited.insert(node_id).second)
                for (const int neighbor_id : embedding.get_adjacency_list(node_id))
                    if (!visited.contains(neighbor_id))
                        stack.push(neighbor_id);
        }
    };
    for (const int node_id : embedding.get_nodes_ids())
        if (!visited.contains(node_id)) {
            components++;
            explore_component(node_id);
        }
    return components;
}

int compute_embedding_genus(
    const size_t number_of_nodes,
    const size_t number_of_edges,
    const size_t number_of_faces,
    const size_t connected_components
) {
    // f - e + v = 2(p - g)
    // f - e + v = 2p - 2g
    // 2g = 2p - f + e - v
    // g = p - (f - e + v) / 2
    const int n = static_cast<int>(number_of_nodes);
    const int e = static_cast<int>(number_of_edges);
    const int f = static_cast<int>(number_of_faces);
    const int p = static_cast<int>(connected_components);
    return p - (f - e + n) / 2;
}

int compute_embedding_genus(const Embedding& embedding) {
    assert(embedding.is_consistent() && "Embedding is not fully undirected");
    const size_t number_of_nodes = embedding.size();
    const size_t number_of_edges = embedding.total_number_of_edges() / 2;
    const size_t number_of_faces = compute_number_of_faces_in_embedding(embedding);
    const size_t connected_components = compute_number_of_connected_components(embedding);
    return compute_embedding_genus(
        number_of_nodes,
        number_of_edges,
        number_of_faces,
        connected_components
    );
}