#include "bridge.hpp"

#include <print>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/subgraph.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

Bridge::Bridge(const SubGraph&& bridge)
    : m_bridge(bridge), m_is_attachment(m_bridge.get_sub_graph()) {}

const Graph& Bridge::get_bridge() const { return m_bridge.get_sub_graph(); }

size_t Bridge::number_of_attachments() const { return m_is_attachment.size(); }

void Bridge::add_attachment(const size_t attachment_id) {
    if (is_attachment(attachment_id))
        return;
    m_is_attachment.add_node(attachment_id);
}

const NodesLabels& Bridge::get_new_id_to_old_id() const { return m_bridge.get_sub_graph_labels(); }

bool Bridge::is_attachment(const size_t node_id) const { return m_is_attachment.has_node(node_id); }

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

void dfs_find_bridges(
    const Graph& graph,
    const size_t node_id,
    NodesContainer& is_node_visited,
    const SubGraph& subgraph,
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
                subgraph,
                edges_in_bridge,
                nodes_in_subgraph
            );
    }
}

void Bridge::find_bridges(
    const Graph& graph,
    const SubGraph& subgraph,
    const NodesContainer& nodes_in_subgraph,
    std::vector<Bridge>& bridges,
    NodesLabels& old_id_to_new_id
) {
    NodesContainer visited(graph);
    for (size_t node_id : subgraph.get_sub_graph().get_nodes_ids()) {
        size_t old_node_id = subgraph.get_sub_graph_labels().get_label(node_id);
        visited.add_node(old_node_id);
    }

    for (size_t node_id : graph.get_nodes_ids())
        if (!visited.has_node(node_id)) {
            std::vector<graph::EdgeId> edges;
            dfs_find_bridges(graph, node_id, visited, subgraph, edges, nodes_in_subgraph);
            bridges.push_back(Bridge::build_bridge(edges, subgraph, old_id_to_new_id));
        }
}

Bridge Bridge::build_bridge(
    std::vector<graph::EdgeId>& edges, const SubGraph& subgraph, NodesLabels& old_id_to_new_id
) {
    DOMUS_ASSERT(
        old_id_to_new_id.get_number_of_labels() == 0,
        "build_bridge: old_id_to_new_id not empty"
    );

    Graph bridge;
    for (const EdgeId edge : edges) {
        const auto [from_id, to_id] = edge.edge;
        if (!old_id_to_new_id.has_label(from_id)) {
            const size_t new_node_id = bridge.add_node();
            old_id_to_new_id.add_label(from_id, new_node_id);
        }
        if (!old_id_to_new_id.has_label(to_id)) {
            const size_t new_node_id = bridge.add_node();
            old_id_to_new_id.add_label(to_id, new_node_id);
        }
    }

    NodesLabels new_id_to_old_id(bridge);
    EdgesLabels edges_labels(edges.size());

    // adding edges
    for (const auto& [old_edge_id, edge] : edges) {
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

    for (const size_t sub_id : subgraph.get_sub_graph().get_nodes_ids()) {
        const size_t old_id = subgraph.get_sub_graph_labels().get_label(sub_id);
        if (old_id_to_new_id.has_label(old_id))
            result.add_attachment(old_id_to_new_id.get_label(old_id));
    }

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

Bridge
Bridge::build_chord(const size_t attachment_1, const size_t attachment_2, const size_t edge_id) {
    Graph chord;
    const size_t new_node_1_id = chord.add_node();
    const size_t new_node_2_id = chord.add_node();
    NodesLabels new_id_to_old_id(chord);

    new_id_to_old_id.add_label(new_node_1_id, attachment_1);
    new_id_to_old_id.add_label(new_node_2_id, attachment_2);

    EdgesLabels edges_labels(1);

    const size_t new_edge_id = chord.add_edge(new_node_1_id, new_node_2_id);
    edges_labels.add_label(new_edge_id, edge_id);

    Bridge result(SubGraph(std::move(chord), std::move(new_id_to_old_id), std::move(edges_labels)));
    result.add_attachment(new_node_1_id);
    result.add_attachment(new_node_2_id);
    return result;
}

void Bridge::find_chords(
    const Graph& graph,
    const SubGraph& subgraph,
    std::vector<Bridge>& bridges,
    const NodesContainer& nodes_in_subgraph
) {
    EdgesContainer edges_in_subgraph(graph);
    for (const size_t sub_node_id : subgraph.get_sub_graph().get_nodes_ids())
        for (const EdgeIter sub_edge : subgraph.get_sub_graph().get_out_edges(sub_node_id)) {
            const size_t edge_id = subgraph.get_sub_graph_edges_labels().get_label(sub_edge.id);
            edges_in_subgraph.add_edge(edge_id);
        }

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

std::vector<Bridge> Bridge::compute(const Graph& graph, const SubGraph& subgraph) {
    std::vector<Bridge> bridges;
    NodesLabels old_id_to_new_id(graph);
    NodesContainer nodes_in_subgraph(graph);
    for (size_t node_id : subgraph.get_sub_graph().get_nodes_ids()) {
        size_t old_node_id = subgraph.get_sub_graph_labels().get_label(node_id);
        nodes_in_subgraph.add_node(old_node_id);
    }
    find_bridges(graph, subgraph, nodes_in_subgraph, bridges, old_id_to_new_id);
    find_chords(graph, subgraph, bridges, nodes_in_subgraph);
    return bridges;
}

const EdgesLabels& Bridge::get_new_edge_id_to_old_id() const {
    return m_bridge.get_sub_graph_edges_labels();
}

} // namespace domus::torus