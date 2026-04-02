#pragma once

#include "graph.hpp"
#include "graph_utilities.hpp"

namespace domus::graph {

class SubGraph {
    Graph m_sub_graph;
    utilities::NodesLabels<size_t> m_sub_graph_labels;
    utilities::EdgesLabels<size_t> m_sub_graph_edges_labels;

  public:
    SubGraph(
        const Graph&& sub_graph,
        const utilities::NodesLabels<size_t>&& sub_graph_labels,
        const utilities::EdgesLabels<size_t>&& sub_graph_edges_labels
    );
    const Graph& get_sub_graph() const;
    const utilities::NodesLabels<size_t>& get_sub_graph_labels() const;
    const utilities::EdgesLabels<size_t>& get_sub_graph_edges_labels() const;
    std::string to_string() const;
    void print() const;
};

} // namespace domus::graph