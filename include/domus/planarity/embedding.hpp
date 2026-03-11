#pragma once

#include <ranges>
#include <string>
#include <unordered_map>

#include "domus/core/circular_sequence.hpp"
#include "domus/core/containers.hpp"
#include "domus/core/graph/graph.hpp"

class Embedding {
    std::unordered_map<size_t, CircularSequence> adjacency_list;
    size_t number_of_edges_m = 0;
    PairIntHashSet m_edges;
    PairIntHashSet m_edges_to_add;

  public:
    explicit Embedding(const Graph& graph);
    void add_edge(size_t from_id, size_t to_id);
    const CircularSequence& get_adjacency_list(size_t node_id) const;
    auto get_nodes_ids() const {
        return adjacency_list |
               std::views::transform([](const auto& pair) -> size_t { return pair.first; });
    }
    std::string to_string() const;
    size_t size() const;
    size_t total_number_of_edges() const;
    bool is_consistent() const;
    void print() const;
};

size_t compute_number_of_faces_in_embedding(const Embedding& embedding);

size_t compute_embedding_genus(
    size_t number_of_nodes,
    size_t number_of_edges,
    size_t number_of_faces,
    size_t connected_components
);

size_t compute_embedding_genus(const Embedding& embedding);

bool is_embedding_planar(const Embedding& embedding);