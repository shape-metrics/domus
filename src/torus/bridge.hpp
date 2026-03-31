#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/graph/subgraph.hpp"

namespace domus::graph {
class Cycle;
}

namespace domus::torus {
using namespace domus::graph;

class Bridge {
  private:
    const SubGraph m_bridge;
    utilities::NodesContainer m_is_attachment;

    Bridge(const SubGraph&& bridge);
    void add_attachment(size_t attachment_id);
    static Bridge build_chord(size_t attachment_1, size_t attachment_2, const size_t edge_id);
    static Bridge build_bridge(
        std::vector<graph::EdgeId>& edges,
        const SubGraph& subgraph,
        utilities::NodesLabels& old_id_to_new_id
    );
    static void find_chords(
        const Graph& graph,
        const SubGraph& subgraph,
        std::vector<Bridge>& bridges,
        const utilities::NodesContainer& nodes_in_subgraph
    );
    static void find_bridges(
        const Graph& graph,
        const SubGraph& subgraph,
        const utilities::NodesContainer& nodes_in_subgraph,
        std::vector<Bridge>& bridges,
        utilities::NodesLabels& old_id_to_new_id
    );

  public:
    const Graph& get_bridge() const;
    const utilities::NodesLabels& get_new_id_to_old_id() const;
    const utilities::EdgesLabels& get_new_edge_id_to_old_id() const;
    size_t number_of_attachments() const;
    bool is_attachment(size_t node_id) const;
    std::string to_string() const;
    void print() const;
    static std::vector<Bridge> compute(const Graph& graph, const SubGraph& subgraph);
};

inline auto compute_all_feet_in_bridge(const Bridge& bridge) {
    auto edges = bridge.get_bridge().get_all_edges();
    return edges | std::views::filter([&bridge](const EdgeId edge) {
               auto [from_id, to_id] = edge.edge;
               return (bridge.is_attachment(from_id)) || (bridge.is_attachment(to_id));
           });
}

} // namespace domus::torus