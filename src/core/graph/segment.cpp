#include "domus/core/graph/segment.hpp"

#include <print>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

Segment::Segment() {
    this->segment = {};
    this->attachments = {};
}

Graph& Segment::get_segment() { return segment; }

const Graph& Segment::get_segment() const { return segment; }

const NodesContainer& Segment::get_attachments() const { return attachments; }

bool Segment::has_attachment(const size_t attachment_id) const {
    return attachments.has_node(attachment_id);
}

void Segment::add_attachment(const size_t attachment_id) { attachments.add_node(attachment_id); }

std::string Segment::to_string() const {
    std::string result = "Segment:\n";
    auto out = std::back_inserter(result);
    result.append(segment.to_string());
    std::format_to(out, "Attachments: ");
    attachments.for_each([&](size_t attachment_id) { std::format_to(out, "{} ", attachment_id); });
    result.push_back('\n');
    return result;
}

void Segment::print() const { println("{}", to_string()); }

bool is_segment_a_path(const Segment& segment) {
    bool is_path = true;
    segment.get_segment().for_each_node([&](size_t node_id) {
        if (!is_path)
            return;
        if (segment.has_attachment(node_id))
            return;
        if (segment.get_segment().get_degree_of_node(node_id) > 2)
            is_path = false;
    });
    return is_path;
}

std::deque<size_t> compute_path_between_attachments(
    const Segment& segment, const size_t attachment_1, const size_t attachment_2
) {
    Int_ToInt_HashMap prev_of_node;
    std::deque<size_t> queue{};
    queue.push_back(attachment_1);
    while (!queue.empty()) {
        const size_t node_id = queue.front();
        queue.pop_front();
        bool keep_exploring = true;
        segment.get_segment().for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!keep_exploring)
                return;
            if (neighbor_id == attachment_2) {
                if (node_id == attachment_1)
                    return;
                prev_of_node.add(neighbor_id, node_id);
                keep_exploring = false;
            }
            if (segment.has_attachment(neighbor_id))
                return;
            if (!prev_of_node.has(neighbor_id)) {
                prev_of_node.add(neighbor_id, node_id);
                queue.push_back(neighbor_id);
            }
        });
        if (prev_of_node.has(attachment_2))
            keep_exploring = false;
    }
    std::deque<size_t> path;
    size_t crawl = attachment_2;
    while (crawl != attachment_1) {
        path.push_front(crawl);
        crawl = prev_of_node.get(crawl);
    }
    path.push_front(crawl);
    return path;
}

void dfs_find_segments(
    const Graph& graph,
    const size_t node_id,
    NodesContainer& is_node_visited,
    std::vector<size_t>& nodes_in_segment,
    const Cycle& cycle,
    std::vector<Edge>& edges_in_segment
) {
    nodes_in_segment.push_back(node_id);
    is_node_visited.add_node(node_id);
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        if (cycle.has_node(neighbor_id)) {
            edges_in_segment.emplace_back(node_id, neighbor_id);
            return;
        }
        if (node_id < neighbor_id)
            edges_in_segment.emplace_back(node_id, neighbor_id);
        if (!is_node_visited.has_node(neighbor_id))
            dfs_find_segments(
                graph,
                neighbor_id,
                is_node_visited,
                nodes_in_segment,
                cycle,
                edges_in_segment
            );
    });
}

void add_cycle_edges(const Cycle& cycle, Segment& segment) {
    cycle.for_each([&](size_t node_id) {
        const size_t next_node_id = cycle.next_of_node(node_id);
        segment.get_segment().add_edge(node_id, next_node_id);
    });
}

Segment
build_segment(const std::vector<size_t>& nodes, std::vector<Edge>& edges, const Cycle& cycle) {
    Segment segment;
    cycle.for_each([&](size_t node_id) { segment.get_segment().add_node(node_id); });
    for (const size_t node_id : nodes)
        segment.get_segment().add_node(node_id);
    // adding edges
    for (const auto& [from_id, to_id] : edges) {
        segment.get_segment().add_edge(from_id, to_id);
        // adding attachment
        if (cycle.has_node(from_id))
            segment.add_attachment(from_id);
        if (cycle.has_node(to_id))
            segment.add_attachment(to_id);
    }
    // adding cycle edges
    add_cycle_edges(cycle, segment);
    return segment;
}

void find_segments(const Graph& graph, const Cycle& cycle, std::vector<Segment>& segments) {
    NodesContainer visited;
    graph.for_each_node([&](size_t node_id) {
        if (cycle.has_node(node_id))
            visited.add_node(node_id);
    });
    graph.for_each_node([&](size_t node_id) {
        if (!visited.has_node(node_id)) {
            std::vector<size_t> nodes; // does NOT contain cycle nodes
            std::vector<Edge> edges;   // does NOT contain edges of the cycle
            dfs_find_segments(graph, node_id, visited, nodes, cycle, edges);
            segments.push_back(build_segment(nodes, edges, cycle));
        }
    });
}

Segment build_chord(const size_t attachment_1, const size_t attachment_2, const Cycle& cycle) {
    Segment chord;
    cycle.for_each([&](size_t node_id) { chord.get_segment().add_node(node_id); });
    add_cycle_edges(cycle, chord);
    // adding chord edge
    chord.get_segment().add_edge(attachment_1, attachment_2);
    chord.add_attachment(attachment_1);
    chord.add_attachment(attachment_2);
    return chord;
}

void find_chords(const Graph& graph, const Cycle& cycle, std::vector<Segment>& segments) {
    cycle.for_each([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (node_id < neighbor_id)
                return;
            if (cycle.has_node(neighbor_id))
                if (neighbor_id != cycle.prev_of_node(node_id) &&
                    neighbor_id != cycle.next_of_node(node_id)) {
                    segments.push_back(build_chord(node_id, neighbor_id, cycle));
                }
        });
    });
}

std::vector<Segment> compute_segments(const Graph& graph, const Cycle& cycle) {
    std::vector<Segment> segments;
    find_segments(graph, cycle, segments);
    find_chords(graph, cycle, segments);
    return segments;
}