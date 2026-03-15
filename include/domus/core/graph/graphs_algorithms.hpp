#pragma once

#include <memory>
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

std::vector<Graph> compute_connected_components(const Graph& graph);

size_t compute_number_of_connected_components(const Graph& graph);

class BiconnectedComponents {
    NodesContainer m_cutvertices;
    std::vector<Graph> m_components;

  public:
    const std::vector<Graph>& get_components() const;
    std::string to_string() const;
    void print() const;
    BiconnectedComponents(NodesContainer&& cutvertices, std::vector<Graph>&& components);
};

BiconnectedComponents compute_biconnected_components(const Graph& graph);

class IBipartitionImpl;

class Bipartition {
  private:
    std::unique_ptr<IBipartitionImpl> m_impl;

  public:
    Bipartition();
    ~Bipartition();
    Bipartition(Bipartition&&) noexcept;
    Bipartition& operator=(Bipartition&&) noexcept;

    void set_side(size_t node_id, bool side);
    bool get_side(size_t node_id) const;
    bool has_node(size_t node_id) const;
    bool are_in_same_side(size_t node_id_1, size_t node_id_2) const;
    std::string to_string() const;
    void print() const;
};

std::optional<Bipartition> compute_bipartition(const Graph& graph);