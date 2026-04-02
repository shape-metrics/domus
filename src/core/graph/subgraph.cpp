#include "domus/core/graph/subgraph.hpp"

#include <print>

namespace domus::graph {

SubGraph::SubGraph(
    const Graph&& sub_graph,
    const utilities::NodesLabels&& sub_graph_labels,
    const utilities::EdgesLabels&& sub_graph_edges_labels
)
    : m_sub_graph(std::move(const_cast<Graph&>(sub_graph))),
      m_sub_graph_labels(std::move(const_cast<utilities::NodesLabels&>(sub_graph_labels))),
      m_sub_graph_edges_labels(
          std::move(const_cast<utilities::EdgesLabels&>(sub_graph_edges_labels))
      ) {}

const Graph& SubGraph::get_sub_graph() const { return m_sub_graph; }

const utilities::NodesLabels& SubGraph::get_sub_graph_labels() const { return m_sub_graph_labels; }

const utilities::EdgesLabels& SubGraph::get_sub_graph_edges_labels() const {
    return m_sub_graph_edges_labels;
}

std::string SubGraph::to_string() const {
    return std::format("{}", m_sub_graph.to_string(true, m_sub_graph_labels, "Subgraph"));
}

void SubGraph::print() const { std::print("{}", to_string()); }

} // namespace domus::graph
