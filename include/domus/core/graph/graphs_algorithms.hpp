#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/tree/tree.hpp"

namespace domus::graph::algorithms {

bool is_graph_connected(const Graph& graph);
std::pair<std::vector<Graph>, utilities::NodesLabels>
compute_connected_components(const Graph& graph);
size_t compute_number_of_connected_components(const Graph& graph);

class SpanningTree {
    const domus::tree::Tree m_tree;
    const utilities::NodesLabels m_edge_ids;
    SpanningTree(const domus::tree::Tree&& tree, const utilities::NodesLabels&& edges_ids);

  public:
    const domus::tree::Tree& get_tree() const;
    const utilities::NodesLabels& get_edge_ids() const;

    static std::optional<SpanningTree> compute(const Graph& graph);
};

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph);
std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph);

std::vector<Cycle> compute_cycle_basis(const Graph& graph);

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph);

class BiconnectedComponents {
    std::vector<size_t> m_cutvertices;
    std::vector<Graph> m_components;
    std::vector<utilities::NodesLabels> m_components_nodes_to_original_nodes;
    BiconnectedComponents(
        std::vector<size_t>&& cutvertices,
        std::vector<Graph>&& components,
        std::vector<utilities::NodesLabels>&& components_to_original_nodes
    );

  public:
    const std::vector<Graph>& get_components() const;
    const utilities::NodesLabels& get_labels_of_component(size_t component_id) const;
    std::string to_string() const;
    void print() const;
    static BiconnectedComponents compute(const Graph& graph);
};

class Bipartition {
  private:
    size_t m_size;
    utilities::NodesLabels m_side;
    Bipartition(const Graph& graph);

  public:
    void set_side(size_t node_id, bool side);

    bool get_side(size_t node_id) const;
    bool has_node(size_t node_id) const;
    bool are_in_same_side(size_t node_id_1, size_t node_id_2) const;

    std::string to_string() const;
    void print() const;

    static std::optional<Bipartition> compute(const Graph& graph);
};

bool is_cycle_in_graph(const Graph& graph, const Cycle& cycle);

bool do_cycles_intersect(const Cycle& cycle_1, const Cycle& cycle_2);

} // namespace domus::graph::algorithms