#include "segment.hpp"

#include <cstddef>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

#include "../core/domus_debug.hpp"

namespace domus::planarity {
using namespace domus::graph;
using namespace domus::graph::utilities;

Segment::Segment(
    const Graph&& segment, const NodesLabels&& labels, const EdgesLabels&& edges_labels
)
    : m_segment(segment), m_new_id_to_old_id(labels), m_is_attachment(m_segment),
      m_edges_labels(edges_labels) {}

const Graph& Segment::get_segment() const { return m_segment; }

void Segment::for_each_attachment(std::function<void(size_t)> f) const {
    for (size_t attachment_id : m_attachments)
        f(attachment_id);
}

size_t Segment::number_of_attachments() const { return m_attachments.size(); }

void Segment::add_attachment(const size_t attachment_id) {
    if (is_attachment(attachment_id))
        return;
    m_attachments.push_back(attachment_id);
    m_is_attachment.add_node(attachment_id);
}

const NodesLabels& Segment::get_new_id_to_old_id() const { return m_new_id_to_old_id; }

bool Segment::is_attachment(const size_t node_id) const {
    return m_is_attachment.has_node(node_id);
}

std::string Segment::to_string() const {
    return m_segment.to_string(true, m_new_id_to_old_id, "Segment");
}

void Segment::print() const { std::println("{}", to_string()); }

bool is_segment_a_path(const Segment& segment) {
    size_t number_degree_3_nodes = 0;
    for (size_t node_id : segment.get_segment().get_node_ids())
        if (segment.get_segment().get_degree_of_node(node_id) > 2)
            number_degree_3_nodes++;
    DOMUS_ASSERT(
        number_degree_3_nodes >= 2,
        "is_segment_a_path: the segment has less than 2 attachments"
    );
    return number_degree_3_nodes == 2;
}

Path compute_path_between_attachments(
    const Segment& segment, const size_t attachment_1, const size_t attachment_2
) {
    NodesLabels edge_id_to_prev(segment.get_segment());
    std::deque<size_t> queue;
    queue.push_back(attachment_1);
    while (!queue.empty()) {
        const size_t node_id = queue.front();
        queue.pop_front();
        for (const auto [edge_id, neighbor_id] : segment.get_segment().get_edges(node_id)) {
            if (neighbor_id == attachment_2) {
                if (node_id == attachment_1)
                    continue;
                edge_id_to_prev.add_label(neighbor_id, edge_id);
                break;
            }
            if (segment.is_attachment(neighbor_id))
                continue;
            if (!edge_id_to_prev.has_label(neighbor_id)) {
                edge_id_to_prev.add_label(neighbor_id, edge_id);
                queue.push_back(neighbor_id);
            }
        }
        if (edge_id_to_prev.has_label(attachment_2))
            break;
    }
    Path path;
    size_t crawl = attachment_2;
    while (crawl != attachment_1) {
        size_t edge_id = edge_id_to_prev.get_label(crawl);
        path.push_back(segment.get_segment(), crawl, edge_id);
        auto [from_id, to_id] = segment.get_segment().get_edge(edge_id);
        crawl = (from_id == crawl) ? to_id : from_id;
    }
    return path;
}

void dfs_find_segments(
    const Graph& graph,
    const size_t node_id,
    NodesContainer& is_node_visited,
    std::vector<size_t>& nodes_in_segment,
    const Cycle& cycle,
    std::vector<graph::EdgeId>& edges_in_segment
) {
    nodes_in_segment.push_back(node_id);
    is_node_visited.add_node(node_id);
    for (const auto [edge_id, neighbor_id] : graph.get_edges(node_id)) {
        if (cycle.has_node_id(neighbor_id)) {
            edges_in_segment.emplace_back(edge_id, graph::Edge{node_id, neighbor_id});
            continue;
        }
        if (node_id < neighbor_id)
            edges_in_segment.emplace_back(edge_id, graph::Edge{node_id, neighbor_id});
        if (!is_node_visited.has_node(neighbor_id))
            dfs_find_segments(
                graph,
                neighbor_id,
                is_node_visited,
                nodes_in_segment,
                cycle,
                edges_in_segment
            );
    }
}

void add_cycle_edges(
    const Cycle& cycle,
    Graph& segment,
    const NodesLabels& old_id_to_new_id,
    EdgesLabels& edges_labels
) {
    for (size_t i = 0; i < cycle.size(); ++i) {
        const size_t old_node_id = cycle.node_id_at(i);
        const size_t old_next_node_id = cycle.node_id_at(i + 1);
        const size_t node_id = old_id_to_new_id.get_label(old_node_id);
        const size_t next_node_id = old_id_to_new_id.get_label(old_next_node_id);
        const size_t old_edge_id = cycle.edge_id_at(i);
        const size_t edge_id = segment.add_edge(node_id, next_node_id);
        edges_labels.add_label(edge_id, old_edge_id);
    }
}

Segment Segment::build_segment(
    const std::vector<size_t>& nodes,
    std::vector<graph::EdgeId>& edges,
    const Cycle& cycle,
    NodesLabels& old_id_to_new_id
) {
    Graph segment;
    for (size_t i = 0; i < nodes.size() + cycle.size(); ++i)
        segment.add_node();
    NodesLabels new_id_to_old_id(segment);
    std::vector<size_t> attachments;
    // important that the cycle nodes have new ids from 0 ... cycle.size()-1
    for (size_t i = 0; i < cycle.size(); ++i) {
        size_t node_id = cycle.node_id_at(i);
        new_id_to_old_id.add_label(i, node_id);
        old_id_to_new_id.update_label(node_id, i);
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        const size_t node_id = nodes[i];
        size_t new_node_id = cycle.size() + i;
        new_id_to_old_id.add_label(new_node_id, node_id);
        old_id_to_new_id.update_label(node_id, new_node_id);
    }
    // adding edges
    EdgesLabels edges_labels(edges.size() + cycle.size());
    for (const auto& [old_edge_id, edge] : edges) {
        const auto [old_from_id, old_to_id] = edge;
        const size_t from_id = old_id_to_new_id.get_label(old_from_id);
        const size_t to_id = old_id_to_new_id.get_label(old_to_id);
        const size_t edge_id = segment.add_edge(from_id, to_id);
        edges_labels.add_label(edge_id, old_edge_id);
        // adding attachment
        if (from_id < cycle.size())
            attachments.push_back(from_id);
        if (to_id < cycle.size())
            attachments.push_back(to_id);
    }
    // adding cycle edges
    add_cycle_edges(cycle, segment, old_id_to_new_id, edges_labels);
    Segment result(std::move(segment), std::move(new_id_to_old_id), std::move(edges_labels));
    for (const size_t attachment_id : attachments)
        result.add_attachment(attachment_id);
    return result;
}

void Segment::find_segments(
    const Graph& graph,
    const Cycle& cycle,
    std::vector<Segment>& segments,
    NodesLabels& old_id_to_new_id
) {
    NodesContainer visited(graph);
    for (size_t node_id : graph.get_node_ids()) {
        old_id_to_new_id.add_label(node_id, graph.get_number_of_nodes());
        if (cycle.has_node_id(node_id))
            visited.add_node(node_id);
    }
    for (size_t node_id : graph.get_node_ids())
        if (!visited.has_node(node_id)) {
            std::vector<size_t> nodes;        // does NOT contain cycle nodes
            std::vector<graph::EdgeId> edges; // does NOT contain edges of the cycle
            dfs_find_segments(graph, node_id, visited, nodes, cycle, edges);
            segments.push_back(Segment::build_segment(nodes, edges, cycle, old_id_to_new_id));
        }
}

Segment Segment::build_chord(
    const size_t attachment_1,
    const size_t attachment_2,
    const size_t edge_id,
    const Cycle& cycle,
    NodesLabels& old_id_to_new_id
) {
    Graph chord;
    for (size_t i = 0; i < cycle.size(); ++i)
        chord.add_node();
    NodesLabels new_id_to_old_id(chord);
    for (size_t i = 0; i < cycle.size(); ++i) {
        const size_t node_id = cycle.node_id_at(i);
        new_id_to_old_id.add_label(i, node_id);
        old_id_to_new_id.update_label(node_id, i);
    }
    EdgesLabels edges_labels(cycle.size() + 1);
    add_cycle_edges(cycle, chord, old_id_to_new_id, edges_labels);
    // adding chord edge
    size_t new_attachment_1 = old_id_to_new_id.get_label(attachment_1);
    size_t new_attachment_2 = old_id_to_new_id.get_label(attachment_2);
    size_t new_edge_id = chord.add_edge(new_attachment_1, new_attachment_2);
    edges_labels.add_label(new_edge_id, edge_id);
    Segment result(std::move(chord), std::move(new_id_to_old_id), std::move(edges_labels));
    result.add_attachment(new_attachment_1);
    result.add_attachment(new_attachment_2);
    return result;
}

void Segment::find_chords(
    const Graph& graph,
    const Cycle& cycle,
    std::vector<Segment>& segments,
    NodesLabels& old_id_to_new_id
) {
    for (size_t i = 0; i < cycle.size(); ++i) {
        size_t node_id = cycle.node_id_at(i);
        size_t next_node_id = cycle.node_id_at(i + 1);
        size_t prev_node_id = cycle.node_id_at(i + cycle.size() - 1);
        for (const auto [edge_id, neighbor_id] : graph.get_edges(node_id)) {
            if (node_id < neighbor_id)
                continue;
            if (cycle.has_node_id(neighbor_id))
                if (neighbor_id != prev_node_id && neighbor_id != next_node_id) {
                    segments.push_back(
                        Segment::build_chord(node_id, neighbor_id, edge_id, cycle, old_id_to_new_id)
                    );
                }
        }
    }
}

std::vector<Segment> Segment::compute(const Graph& graph, const Cycle& cycle) {
    std::vector<Segment> segments;
    NodesLabels old_id_to_new_id(graph);
    find_segments(graph, cycle, segments, old_id_to_new_id);
    find_chords(graph, cycle, segments, old_id_to_new_id);
    return segments;
}

const EdgesLabels& Segment::get_edge_labels() const { return m_edges_labels; }

} // namespace domus::planarity