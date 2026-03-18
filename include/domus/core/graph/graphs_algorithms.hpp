#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

bool is_graph_connected(const Graph& graph);

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph);

std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph);

std::vector<Cycle> compute_cycle_basis(const Graph& graph);

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph);

std::pair<std::vector<Graph>, NodesLabels> compute_connected_components(const Graph& graph);

size_t compute_number_of_connected_components(const Graph& graph);

class BiconnectedComponents {
    std::vector<size_t> m_cutvertices;
    std::vector<Graph> m_components;
    std::vector<NodesLabels> m_components_nodes_to_original_nodes;

  public:
    const std::vector<Graph>& get_components() const;
    std::string to_string() const;
    void print() const;
    BiconnectedComponents(
        std::vector<size_t>&& cutvertices,
        std::vector<Graph>&& components,
        std::vector<NodesLabels>&& components_to_original_nodes
    );
};

BiconnectedComponents compute_biconnected_components(const Graph& graph);

class Bipartition {
  private:
    size_t m_size;
    NodesLabels m_side;

  public:
    Bipartition(const Graph& graph);

    void set_side(size_t node_id, bool side);
    bool get_side(size_t node_id) const;
    bool has_node(size_t node_id) const;
    bool are_in_same_side(size_t node_id_1, size_t node_id_2) const;
    std::string to_string() const;
    void print() const;
};

std::optional<Bipartition> compute_bipartition(const Graph& graph);
