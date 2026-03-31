#pragma once

#include "graph.hpp"
#include "graph_utilities.hpp"

namespace domus::graph {

class SubGraph {
    Graph m_sub_graph;
    utilities::NodesLabels m_sub_graph_labels;
    utilities::EdgesLabels m_sub_graph_edges_labels;

  public:
    SubGraph(
        const Graph&& sub_graph,
        const utilities::NodesLabels&& sub_graph_labels,
        const utilities::EdgesLabels&& sub_graph_edges_labels
    );
    const Graph& get_sub_graph() const;
    const utilities::NodesLabels& get_sub_graph_labels() const;
    const utilities::EdgesLabels& get_sub_graph_edges_labels() const;
    std::string to_string() const;
    void print() const;
};

} // namespace domus::graph