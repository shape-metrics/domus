#pragma once

#include <vector>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/subgraph.hpp"

namespace domus::graph {
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class BridgeFactory;

class Bridge {
    friend class BridgeFactory;

  private:
    const graph::SubGraph m_bridge;
    graph::utilities::NodesContainer m_is_attachment;
    Bridge(const graph::SubGraph&& bridge);

  public:
    const graph::Graph& get_bridge() const;
    const graph::utilities::NodesLabels<size_t>& get_new_id_to_old_id() const;
    const graph::utilities::EdgesLabels<size_t>& get_new_edge_id_to_old_id() const;
    size_t number_of_attachments() const;
    bool is_attachment(size_t node_id) const;
    std::string to_string() const;
    void print() const;
    static std::vector<Bridge> compute(const graph::Graph& graph, const graph::SubGraph& subgraph);
    static std::vector<Bridge>
    compute(const graph::Graph& graph, const graph::Embedding& embedding);
};

inline auto compute_all_feet_in_bridge(const Bridge& bridge) {
    auto edges = bridge.get_bridge().get_all_edges();
    return std::views::all(std::move(edges)) |
           std::views::filter([&bridge](const graph::EdgeId edge) {
               auto [from_id, to_id] = edge.edge;
               return (bridge.is_attachment(from_id)) || (bridge.is_attachment(to_id));
           });
}

} // namespace domus::torus