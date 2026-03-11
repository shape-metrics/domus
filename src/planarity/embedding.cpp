#include "domus/planarity/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <cassert>
#include <functional>
#include <print>
#include <stack>

using namespace std;

Embedding::Embedding(const Graph& graph) {
    graph.for_each_node([&](size_t node_id) { adjacency_list[node_id]; });
}

void Embedding::add_edge(size_t from_id, size_t to_id) {
    assert(!m_edges.has(from_id, to_id) && "Edge already exists");
    m_edges.add(from_id, to_id);
    if (m_edges_to_add.has(from_id, to_id)) {
        m_edges_to_add.erase(from_id, to_id);
    } else {
        m_edges_to_add.add(to_id, from_id);
    }
    adjacency_list.at(from_id).append(to_id);
    number_of_edges_m++;
}

bool Embedding::is_consistent() const { return m_edges_to_add.empty(); }

const CircularSequence& Embedding::get_adjacency_list(size_t node_id) const {
    return adjacency_list.at(node_id);
}

string Embedding::to_string() const {
    string result;
    // for (const auto& [node_id, neighbors] : adjacency_list) {
    //     result += "Node " + std::to_string(node_id) + " neighbors:";
    //     for (size_t neighbor_id : neighbors)
    //         result += " " + std::to_string(neighbor_id);
    //     result += "\n";
    // }
    return result;
}

size_t Embedding::size() const { return adjacency_list.size(); }

size_t Embedding::total_number_of_edges() const { return number_of_edges_m; }

void Embedding::print() const { std::print("{}", to_string()); }

size_t compute_number_of_faces_in_embedding(const Embedding& embedding) {
    size_t number_of_faces = 0;
    PairIntHashSet visited_edges; // visited oriented edges
    for (size_t node_id : embedding.get_nodes_ids()) {
        embedding.get_adjacency_list(node_id).for_each([&](size_t neighbor_id) {
            if (visited_edges.has(node_id, neighbor_id))
                return;
            ++number_of_faces;
            size_t current_node = node_id;
            size_t next_node = neighbor_id;
            visited_edges.add(node_id, neighbor_id);
            while (true) {
                size_t successor =
                    embedding.get_adjacency_list(next_node).next_element(current_node);
                if (visited_edges.has(next_node, successor))
                    break;
                visited_edges.add(next_node, successor);
                current_node = next_node;
                next_node = successor;
                if (current_node == node_id && next_node == neighbor_id)
                    break;
            }
        });
    }
    return number_of_faces;
}

bool is_embedding_planar(const Embedding& embedding) {
    return compute_embedding_genus(embedding) == 0;
}

size_t compute_number_of_connected_components(const Embedding& embedding) {
    assert(embedding.is_consistent() && "Embedding is not fully undirected");
    NodesContainer visited;
    size_t components = 0;
    const function<void(size_t)> explore_component = [&](size_t start_node_id) {
        stack<size_t> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            size_t node_id = stack.top();
            stack.pop();
            if (!visited.has_node(node_id)) {
                visited.add_node(node_id);
                embedding.get_adjacency_list(node_id).for_each([&stack,
                                                                &visited](size_t neighbor_id) {
                    if (!visited.has_node(neighbor_id))
                        stack.push(neighbor_id);
                });
            }
        }
    };
    for (size_t node_id : embedding.get_nodes_ids())
        if (!visited.has_node(node_id)) {
            components++;
            explore_component(node_id);
        }
    return components;
}

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
) {
    // f - e + v = 2(p - g)
    // f - e + v = 2p - 2g
    // 2g = 2p - f + e - v
    // g = p - (f - e + v) / 2
    int n = static_cast<int>(number_of_nodes);
    int e = static_cast<int>(number_of_edges);
    int f = static_cast<int>(number_of_faces);
    int p = static_cast<int>(connected_components);
    int genus = p - (f - e + n) / 2;
    assert(genus >= 0 && "compute_embedding_genus: genus is negative");
    return static_cast<size_t>(genus);
}

size_t compute_embedding_genus(const Embedding& embedding) {
    assert(
        embedding.is_consistent() && "compute_embedding_genus: embedding is not fully undirected"
    );
    size_t number_of_nodes = embedding.size();
    size_t number_of_edges = embedding.total_number_of_edges() / 2;
    size_t number_of_faces = compute_number_of_faces_in_embedding(embedding);
    size_t connected_components = compute_number_of_connected_components(embedding);
    return compute_embedding_genus(
        number_of_nodes,
        number_of_edges,
        number_of_faces,
        connected_components
    );
}