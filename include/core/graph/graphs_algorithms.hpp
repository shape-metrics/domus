#ifndef MY_GRAPHS_ALGORITHMS_H
#define MY_GRAPHS_ALGORITHMS_H

#include <optional>
#include <vector>

#include "core/graph/cycle.hpp"
#include "core/graph/graph.hpp"

bool is_graph_connected(const Graph& graph);

bool is_graph_undirected(const Graph& graph);

std::vector<Cycle> compute_all_cycles_with_node_in_undirected_graph(
    const Graph& graph, const GraphNode& node,
    std::unordered_set<int>& taboo_nodes);

std::vector<Cycle> compute_all_cycles_in_undirected_graph(const Graph& graph);

std::optional<Cycle> find_a_cycle_directed_graph(const Graph& graph);

std::vector<Cycle> compute_cycle_basis(const Graph& graph);

std::vector<int> make_topological_ordering(const Graph& graph);

int compute_embedding_genus(int number_of_nodes, int number_of_edges,
                            int number_of_faces, int connected_components);

bool are_cycles_equivalent(const Cycle& cycle1, const Cycle& cycle2);

std::vector<std::unique_ptr<Graph>> compute_connected_components(
    const Graph& graph);

struct BiconnectedComponents {
  std::unordered_set<int> cutvertices;
  std::vector<std::unique_ptr<Graph>> components;
  std::string to_string() const;
  void print() const;
};

BiconnectedComponents compute_biconnected_components(const Graph& graph);

std::pair<std::unique_ptr<Graph>, GraphEdgeHashSet>
compute_maximal_degree_4_subgraph(const Graph& graph);

#endif