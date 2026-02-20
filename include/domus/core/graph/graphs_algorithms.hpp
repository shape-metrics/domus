#ifndef MY_GRAPHS_ALGORITHMS_H
#define MY_GRAPHS_ALGORITHMS_H

#include <expected>
#include <optional>
#include <stddef.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"

bool is_graph_connected(const UndirectedGraph& graph);

std::vector<Cycle> compute_all_cycles_with_node_in_graph(
    const UndirectedGraph& graph, int node_id, const std::unordered_set<int>& taboo_nodes
);

std::vector<Cycle> compute_all_cycles_in_graph(const UndirectedGraph& graph);

std::optional<Cycle> find_a_cycle_in_graph(const UndirectedGraph& graph);

std::optional<Cycle> find_a_cycle_in_graph(const DirectedGraph& graph);

std::expected<std::vector<Cycle>, std::string> compute_cycle_basis(const UndirectedGraph& graph);

std::optional<std::vector<int>> make_topological_ordering(const DirectedGraph& graph);

bool are_cycles_equivalent(const Cycle& cycle1, const Cycle& cycle2);

std::vector<UndirectedGraph> compute_connected_components(const UndirectedGraph& graph);

size_t compute_number_of_connected_components(const UndirectedGraph& graph);

class BiconnectedComponents {
    std::unordered_set<int> m_cutvertices;
    std::vector<UndirectedGraph> m_components;

  public:
    std::unordered_set<int>& get_cutvertices();
    const std::unordered_set<int>& get_cutvertices() const;
    const std::vector<UndirectedGraph>& get_components() const;
    std::string to_string() const;
    void print() const;
    BiconnectedComponents(
        std::unordered_set<int>&& cutvertices, std::vector<UndirectedGraph>&& components
    );
};

BiconnectedComponents compute_biconnected_components(const UndirectedGraph& graph);

std::optional<std::unordered_map<int, bool>> compute_bipartition(const UndirectedGraph& graph);

#endif