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

std::pair<std::vector<Graph>, utilities::NodesLabels<size_t>>
compute_connected_components(const Graph& graph);

template <UndirectedGraphLike G> size_t compute_number_of_connected_components(const G& graph);

class SpanningTree {
    const domus::tree::Tree m_tree;
    const utilities::NodesLabels<size_t> m_edge_ids;
    SpanningTree(const domus::tree::Tree&& tree, const utilities::NodesLabels<size_t>&& edges_ids);

  public:
    const domus::tree::Tree& get_tree() const;
    const utilities::NodesLabels<size_t>& get_edge_ids() const;

    static std::optional<SpanningTree> compute(const Graph& graph);
};

std::optional<Path>
find_shortest_path_between_nodes(const Graph& graph, size_t node_id_1, size_t node_id_2);

std::optional<Cycle> find_an_undirected_cycle_in_graph(const Graph& graph);
std::optional<Cycle> find_a_directed_cycle_in_graph(const Graph& graph);

std::vector<Cycle> compute_cycle_basis(const Graph& graph);

std::optional<std::vector<size_t>> make_topological_ordering(const Graph& graph);

class BiconnectedComponents {
    std::vector<size_t> m_cutvertices;
    std::vector<Graph> m_components;
    std::vector<utilities::NodesLabels<size_t>> m_components_nodes_to_original_nodes;
    BiconnectedComponents(
        std::vector<size_t>&& cutvertices,
        std::vector<Graph>&& components,
        std::vector<utilities::NodesLabels<size_t>>&& components_to_original_nodes
    );

  public:
    const std::vector<Graph>& get_components() const;
    const utilities::NodesLabels<size_t>& get_labels_of_component(size_t component_id) const;
    std::string to_string() const;
    void print() const;
    static BiconnectedComponents compute(const Graph& graph);
};

class Bipartition {
  private:
    size_t m_size;
    utilities::NodesLabels<size_t> m_side;
    Bipartition(const Graph& graph);

  public:
    void set_side(size_t node_id, bool side);

    bool get_side(size_t node_id) const;
    bool has_node(size_t node_id) const;
    bool are_in_same_side(size_t node_id_1, size_t node_id_2) const;

    std::string to_string() const;
    void print() const;

    static std::optional<Bipartition> compute(const Graph& graph);
    static bool is_bipartite(const Graph& graph);
};

bool is_cycle_in_graph(const Graph& graph, const Cycle& cycle);

std::optional<size_t> do_cycles_intersect(const Cycle& cycle_1, const Cycle& cycle_2);

struct StrongConnectedComponents {
    const std::vector<std::vector<size_t>> sccs;
    const utilities::NodesLabels<size_t> node_to_scc_id;
    static StrongConnectedComponents compute(const Graph& graph);

  private:
    StrongConnectedComponents(
        const std::vector<std::vector<size_t>>&& sccs,
        const utilities::NodesLabels<size_t>&& node_to_scc_id
    );
};

/*
 * templates implementations
 */

template <UndirectedGraphLike G> size_t compute_number_of_connected_components(const G& graph) {
    utilities::NodesContainer visited(graph);
    size_t components = 0;
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (!visited.has_node(node_id)) {
            components++;
            std::stack<size_t> stack;
            stack.push(node_id);
            while (!stack.empty()) {
                const size_t n_id = stack.top();
                stack.pop();
                if (!visited.has_node(n_id)) {
                    visited.add_node(n_id);
                    for (const size_t neighbor_id : graph.get_neighbors(n_id))
                        if (!visited.has_node(neighbor_id))
                            stack.push(neighbor_id);
                }
            }
        }
    }
    return components;
}

} // namespace domus::graph::algorithms
