#include "domus/planarity/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include <cstddef>
#include <print>
#include <unordered_set>

#include "../core/domus_debug.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

namespace domus::planarity {
using graph::Graph;

const Graph& Embedding::get_graph() const { return m_graph; }

Embedding::Embedding(const Graph& graph) : m_graph(graph) {
    graph.for_each_node([&](size_t) { adjacency_list.push_back({}); });
}

size_t Embedding::next_element_in_adjacency_list(size_t node_id, size_t element) const {
    const size_t pos = adjacency_list[node_id].element_position(element).value();

    return adjacency_list[node_id][pos + 1];
}

size_t Embedding::get_node_degree(size_t node_id) const {
    return adjacency_list.at(node_id).size();
}

void Embedding::add_edge(size_t from_id, size_t to_id) {
    adjacency_list.at(from_id).append(to_id);
    number_of_edges_m++;
}

std::string Embedding::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Embedding:\n");
    for_each_node([&](size_t node_id) {
        std::format_to(out, "{}: [ ", node_id);
        for_each_neighbor(node_id, [&](size_t neighbor_id) {
            std::format_to(out, "{} ", neighbor_id);
        });
        std::format_to(out, "]\n");
    });
    return result;
}

size_t Embedding::size() const { return adjacency_list.size(); }

void Embedding::for_each_node(std::function<void(size_t)> func) const {
    m_graph.for_each_node(func);
}

void Embedding::for_each_neighbor(size_t node_id, std::function<void(size_t)> func) const {
    adjacency_list.at(node_id).for_each(func);
}

size_t Embedding::total_number_of_edges() const { return number_of_edges_m; }

void Embedding::print() const { std::print("{}", to_string()); }

struct edge_hash {
    size_t operator()(const graph::Edge& edge) const {
        size_t h1 = std::hash<size_t>{}(edge.from_id);
        size_t h2 = std::hash<size_t>{}(edge.to_id);
        size_t mult = h2 * 0x9e3779b9;
        return h1 ^ (mult + (h1 << 6) + (h1 >> 2));
    }
};

size_t compute_number_of_faces_in_embedding(const Embedding& embedding) {
    size_t number_of_faces = 0;
    std::unordered_set<graph::Edge, edge_hash> visited_edges; // visited oriented edges
    embedding.for_each_node([&](size_t node_id) {
        embedding.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (visited_edges.contains({node_id, neighbor_id}))
                return;
            ++number_of_faces;
            size_t current_node = node_id;
            size_t next_node = neighbor_id;
            visited_edges.insert({node_id, neighbor_id});
            while (true) {
                size_t successor =
                    embedding.next_element_in_adjacency_list(next_node, current_node);
                if (visited_edges.contains({next_node, successor}))
                    break;
                visited_edges.insert({next_node, successor});
                current_node = next_node;
                next_node = successor;
                if (current_node == node_id && next_node == neighbor_id)
                    break;
            }
        });
    });
    return number_of_faces;
}

bool is_embedding_planar(const Embedding& embedding) {
    return compute_embedding_genus(embedding) == 0;
}

// This function verifies that for every edge from_id-to_id there is the edge to_id-from_id
// It is intended to be used only for debug purposes
bool Embedding::is_consistent() const {
    std::unordered_set<graph::Edge, edge_hash> edges;
    for_each_node([this, &edges](size_t node_id) {
        for_each_neighbor(node_id, [&edges, node_id](size_t neighbor_id) {
            if (edges.contains({neighbor_id, node_id}))
                edges.erase({neighbor_id, node_id});
            else
                edges.insert({node_id, neighbor_id});
        });
    });
    return edges.empty();
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
    DOMUS_ASSERT(genus >= 0, "compute_embedding_genus: genus is negative");
    return static_cast<size_t>(genus);
}

size_t compute_embedding_genus(const Embedding& embedding) {
    DOMUS_ASSERT(
        embedding.is_consistent(),
        "compute_embedding_genus: embedding is not fully undirected"
    );
    size_t number_of_nodes = embedding.size();
    size_t number_of_edges = embedding.total_number_of_edges() / 2;
    size_t number_of_faces = compute_number_of_faces_in_embedding(embedding);
    size_t connected_components =
        graph::algorithms::compute_number_of_connected_components(embedding.get_graph());
    return compute_embedding_genus(
        number_of_nodes,
        number_of_edges,
        number_of_faces,
        connected_components
    );
}

} // namespace domus::planarity