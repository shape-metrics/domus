#include "domus/torus/bridge.hpp"

#include <print>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/graph/subgraph.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

Bridge::Bridge(const SubGraph&& bridge)
    : m_bridge(bridge), m_is_attachment(m_bridge.get_sub_graph()) {}

const Graph& Bridge::get_bridge() const { return m_bridge.get_sub_graph(); }

const std::vector<size_t> Bridge::get_attachments() const { return m_attachments; }

size_t Bridge::number_of_attachments() const { return m_is_attachment.size(); }

const NodesLabels<size_t>& Bridge::get_new_id_to_old_id() const {
    return m_bridge.get_sub_graph_labels();
}

bool Bridge::is_attachment(const size_t node_id) const { return m_is_attachment.has_node(node_id); }

const EdgesLabels<size_t>& Bridge::get_new_edge_id_to_old_id() const {
    return m_bridge.get_sub_graph_edges_labels();
}

std::string Bridge::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);

    std::format_to(
        out,
        "{}",
        m_bridge.get_sub_graph().to_string(true, m_bridge.get_sub_graph_labels(), "Bridge")
    );

    std::format_to(out, "Attachments:");
    for (size_t node_id : m_bridge.get_sub_graph().get_nodes_ids())
        if (is_attachment(node_id))
            std::format_to(out, " {}", m_bridge.get_sub_graph_labels().get_label(node_id));
    std::format_to(out, "\n");

    return result;
}

void Bridge::print() const { std::println("{}", to_string()); }

class BridgeFactory {
    static Bridge create_bridge(const graph::SubGraph&& bridge);
    static void add_attachment(Bridge& bridge, size_t attachment_id);
    static Bridge build_chord(size_t attachment_1, size_t attachment_2, const size_t edge_id);
    static Bridge build_bridge(
        std::vector<graph::EdgeId>& edges,
        const NodesContainer& nodes_in_subgraph,
        graph::utilities::NodesLabels<size_t>& old_id_to_new_id
    );
    static void find_chords(
        const graph::Graph& graph,
        std::vector<Bridge>& bridges,
        const graph::utilities::NodesContainer& nodes_in_subgraph,
        const graph::utilities::EdgesContainer& edges_in_subgraph
    );
    static void find_bridges(
        const graph::Graph& graph,
        const graph::utilities::NodesContainer& nodes_in_subgraph,
        std::vector<Bridge>& bridges,
        graph::utilities::NodesLabels<size_t>& old_id_to_new_id
    );

  public:
    static std::vector<Bridge> compute(const graph::Graph& graph, const graph::SubGraph& subgraph);
    static std::vector<Bridge> compute(const graph::Graph& graph, const graph::Embedding& subgraph);
};

void BridgeFactory::add_attachment(Bridge& bridge, const size_t attachment_id) {
    if (bridge.is_attachment(attachment_id))
        return;
    bridge.m_is_attachment.add_node(attachment_id);
    bridge.m_attachments.push_back(attachment_id);
}

void dfs_find_bridges(
    const Graph& graph,
    const size_t node_id,
    NodesContainer& is_node_visited,
    std::vector<graph::EdgeId>& edges_in_bridge,
    const NodesContainer& nodes_in_subgraph
) {
    is_node_visited.add_node(node_id);
    for (const EdgeIter edge : graph.get_edges(node_id)) {
        const size_t neighbor_id = edge.neighbor_id;
        if (nodes_in_subgraph.has_node(neighbor_id)) {
            edges_in_bridge.emplace_back(edge.id, graph::Edge{node_id, neighbor_id});
            continue;
        }
        if (node_id < neighbor_id)
            edges_in_bridge.emplace_back(edge.id, graph::Edge{node_id, neighbor_id});
        if (!is_node_visited.has_node(neighbor_id))
            dfs_find_bridges(
                graph,
                neighbor_id,
                is_node_visited,
                edges_in_bridge,
                nodes_in_subgraph
            );
    }
}

void BridgeFactory::find_bridges(
    const Graph& graph,
    const NodesContainer& nodes_in_subgraph,
    std::vector<Bridge>& bridges,
    NodesLabels<size_t>& old_id_to_new_id
) {
    NodesContainer is_node_visited(graph);
    for (size_t node_id : graph.get_nodes_ids())
        if (nodes_in_subgraph.has_node(node_id))
            is_node_visited.add_node(node_id);

    for (size_t node_id : graph.get_nodes_ids())
        if (!is_node_visited.has_node(node_id)) {
            std::vector<graph::EdgeId> edges_in_bridge;
            dfs_find_bridges(graph, node_id, is_node_visited, edges_in_bridge, nodes_in_subgraph);
            bridges.push_back(
                BridgeFactory::build_bridge(edges_in_bridge, nodes_in_subgraph, old_id_to_new_id)
            );
        }
}

Bridge BridgeFactory::build_bridge(
    std::vector<graph::EdgeId>& edges_in_bridge,
    const NodesContainer& nodes_in_subgraph,
    NodesLabels<size_t>& old_id_to_new_id
) {
    DOMUS_ASSERT(
        old_id_to_new_id.get_number_of_labels() == 0,
        "build_bridge: old_id_to_new_id not empty"
    );

    Graph bridge;
    std::vector<size_t> attachments;
    for (const EdgeId edge : edges_in_bridge) {
        const auto [from_id, to_id] = edge.edge;
        if (!old_id_to_new_id.has_label(from_id)) {
            const size_t new_node_id = bridge.add_node();
            old_id_to_new_id.add_label(from_id, new_node_id);
            if (nodes_in_subgraph.has_node(from_id)) {
                attachments.push_back(new_node_id);
            }
        }
        if (!old_id_to_new_id.has_label(to_id)) {
            const size_t new_node_id = bridge.add_node();
            old_id_to_new_id.add_label(to_id, new_node_id);
            if (nodes_in_subgraph.has_node(to_id)) {
                attachments.push_back(new_node_id);
            }
        }
    }

    NodesLabels<size_t> new_id_to_old_id(bridge);
    EdgesLabels<size_t> edges_labels(edges_in_bridge.size());

    // adding edges
    for (const auto& [old_edge_id, edge] : edges_in_bridge) {
        const auto [old_from_id, old_to_id] = edge;
        const size_t from_id = old_id_to_new_id.get_label(old_from_id);
        const size_t to_id = old_id_to_new_id.get_label(old_to_id);
        const size_t edge_id = bridge.add_edge(from_id, to_id);
        edges_labels.add_label(edge_id, old_edge_id);

        new_id_to_old_id.add_or_update_label(from_id, old_from_id);
        new_id_to_old_id.add_or_update_label(to_id, old_to_id);
    }

    Bridge result(
        SubGraph(std::move(bridge), std::move(new_id_to_old_id), std::move(edges_labels))
    );

    for (const size_t node_id : attachments)
        add_attachment(result, node_id);

    for (const size_t node_id : result.get_bridge().get_nodes_ids()) {
        const size_t old_id = result.get_new_id_to_old_id().get_label(node_id);
        old_id_to_new_id.erase_label(old_id);
    }

    DOMUS_ASSERT(
        old_id_to_new_id.get_number_of_labels() == 0,
        "build_bridge: old_id_to_new_id not empty"
    );

    return result;
}

Bridge BridgeFactory::build_chord(
    const size_t attachment_1, const size_t attachment_2, const size_t edge_id
) {
    Graph chord;
    const size_t new_node_1_id = chord.add_node();
    const size_t new_node_2_id = chord.add_node();
    NodesLabels<size_t> new_id_to_old_id(chord);

    new_id_to_old_id.add_label(new_node_1_id, attachment_1);
    new_id_to_old_id.add_label(new_node_2_id, attachment_2);

    EdgesLabels<size_t> edges_labels(1);

    const size_t new_edge_id = chord.add_edge(new_node_1_id, new_node_2_id);
    edges_labels.add_label(new_edge_id, edge_id);

    Bridge result(SubGraph(std::move(chord), std::move(new_id_to_old_id), std::move(edges_labels)));
    add_attachment(result, new_node_1_id);
    add_attachment(result, new_node_2_id);
    return result;
}

void BridgeFactory::find_chords(
    const Graph& graph,
    std::vector<Bridge>& bridges,
    const NodesContainer& nodes_in_subgraph,
    const EdgesContainer& edges_in_subgraph
) {
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (!nodes_in_subgraph.has_node(node_id))
            continue;
        for (const EdgeIter edge : graph.get_out_edges(node_id)) {
            const size_t neighbor_id = edge.neighbor_id;
            if (!nodes_in_subgraph.has_node(neighbor_id))
                continue;
            if (!edges_in_subgraph.has_edge(edge.id))
                bridges.push_back(build_chord(node_id, neighbor_id, edge.id));
        }
    }
}

std::vector<Bridge> BridgeFactory::compute(const Graph& graph, const SubGraph& subgraph) {
    std::vector<Bridge> bridges;
    NodesLabels<size_t> old_id_to_new_id(graph);
    NodesContainer nodes_in_subgraph(graph);
    for (const size_t node_id : subgraph.get_sub_graph().get_nodes_ids()) {
        size_t old_node_id = subgraph.get_sub_graph_labels().get_label(node_id);
        nodes_in_subgraph.add_node(old_node_id);
    }
    EdgesContainer edges_in_subgraph(graph);
    for (const size_t sub_node_id : subgraph.get_sub_graph().get_nodes_ids())
        for (const EdgeIter sub_edge : subgraph.get_sub_graph().get_out_edges(sub_node_id)) {
            const size_t edge_id = subgraph.get_sub_graph_edges_labels().get_label(sub_edge.id);
            edges_in_subgraph.add_edge(edge_id);
        }
    find_bridges(graph, nodes_in_subgraph, bridges, old_id_to_new_id);
    find_chords(graph, bridges, nodes_in_subgraph, edges_in_subgraph);
    return bridges;
}

std::vector<Bridge> BridgeFactory::compute(const Graph& graph, const Embedding& embedding) {
    std::vector<Bridge> bridges;
    NodesLabels<size_t> old_id_to_new_id(graph);
    NodesContainer nodes_in_subgraph(graph);
    for (const size_t node_id : embedding.get_nodes_ids())
        if (embedding.get_degree_of_node(node_id) != 0)
            nodes_in_subgraph.add_node(node_id);
    EdgesContainer edges_in_subgraph(graph);
    for (const size_t node_id : embedding.get_nodes_ids())
        for (const EdgeIter edge : embedding.get_edges(node_id))
            if (node_id < edge.neighbor_id)
                edges_in_subgraph.add_edge(edge.id);
    find_bridges(graph, nodes_in_subgraph, bridges, old_id_to_new_id);
    find_chords(graph, bridges, nodes_in_subgraph, edges_in_subgraph);
    return bridges;
}

std::vector<Bridge> Bridge::compute(const Graph& graph, const SubGraph& subgraph) {
    return BridgeFactory::compute(graph, subgraph);
}

std::vector<Bridge> Bridge::compute(const Graph& graph, const Embedding& embedding) {
    return BridgeFactory::compute(graph, embedding);
}

} // namespace domus::torus
