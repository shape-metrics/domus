#include "domus/core/graph/segment.hpp"

#include <format>
#include <initializer_list>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <utility>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"

Segment::Segment() {
    this->segment = {};
    this->attachments = {};
}

UndirectedGraph& Segment::get_segment() {
    return segment;
}

const UndirectedGraph& Segment::get_segment() const {
    return segment;
}

const std::unordered_set<int>& Segment::get_attachments() const {
    return attachments;
}

bool Segment::has_attachment(const int attachment_id) const {
    return attachments.contains(attachment_id);
}

void Segment::add_attachment(const int attachment_id) {
    attachments.insert(attachment_id);
}

std::string Segment::to_string() const {
    std::string result = "Segment:\n";
    result += segment.to_string();
    result += "Attachments: ";
    for (const int attachment_id : attachments)
        result += std::to_string(attachment_id) + " ";
    result += "\n";
    return result;
}

void Segment::print() const {
    std::cout << to_string();
}

bool is_segment_a_path(const Segment& segment) {
    for (int node_id : segment.get_segment().get_nodes_ids()) {
        if (segment.has_attachment(node_id))
            continue;
        if (segment.get_segment().get_degree_of_node(node_id) > 2)
            return false;
    }
    return true;
}

std::list<int> compute_path_between_attachments(
    const Segment& segment,
    const int attachment_1,
    const int attachment_2) {
    std::unordered_map<int, int> prev_of_node;
    std::list<int> queue{};
    queue.push_back(attachment_1);
    while (!queue.empty()) {
        const int node_id = queue.front();
        queue.pop_front();
        for (int neighbor_id :
             segment.get_segment().get_neighbors_of_node(node_id)) {
            if (neighbor_id == attachment_2) {
                if (node_id == attachment_1)
                    continue;
                prev_of_node[neighbor_id] = node_id;
                break;
            }
            if (segment.has_attachment(neighbor_id))
                continue;
            if (!prev_of_node.contains(neighbor_id)) {
                prev_of_node[neighbor_id] = node_id;
                queue.push_back(neighbor_id);
            }
        }
        if (prev_of_node.contains(attachment_2))
            break;
    }
    std::list<int> path;
    int crawl = attachment_2;
    while (crawl != attachment_1) {
        path.push_front(crawl);
        crawl = prev_of_node[crawl];
    }
    path.push_front(crawl);
    return path;
}

void dfs_find_segments(
    const UndirectedGraph& graph,
    const int node_id,
    std::unordered_set<int>& is_node_visited,
    std::vector<int>& nodes_in_segment,
    const Cycle& cycle,
    std::vector<std::pair<int, int>>& edges_in_segment) {
    nodes_in_segment.push_back(node_id);
    is_node_visited.insert(node_id);
    for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
        if (cycle.has_node(neighbor_id)) {
            edges_in_segment.emplace_back(node_id, neighbor_id);
            continue;
        }
        if (node_id < neighbor_id)
            edges_in_segment.emplace_back(node_id, neighbor_id);
        if (!is_node_visited.contains(neighbor_id))
            dfs_find_segments(
                graph,
                neighbor_id,
                is_node_visited,
                nodes_in_segment,
                cycle,
                edges_in_segment);
    }
}

void add_cycle_edges(const Cycle& cycle, Segment& segment) {
    for (const int node_id : cycle) {
        const int next_node_id = cycle.next_of_node(node_id);
        segment.get_segment().add_edge(node_id, next_node_id);
    }
}

Segment build_segment(
    const std::vector<int>& nodes,
    std::vector<std::pair<int, int>>& edges,
    const Cycle& cycle) {
    Segment segment;
    for (const int node_id : cycle)
        segment.get_segment().add_node(node_id);
    for (const int node_id : nodes)
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

void find_segments(
    const UndirectedGraph& graph,
    const Cycle& cycle,
    std::vector<Segment>& segments) {
    std::unordered_set<int> visited;
    for (const int node_id : graph.get_nodes_ids())
        if (cycle.has_node(node_id))
            visited.insert(node_id);
    for (const int node_id : graph.get_nodes_ids()) {
        if (!visited.contains(node_id)) {
            std::vector<int> nodes;  // does NOT contain cycle nodes
            std::vector<std::pair<int, int>>
                edges;  // does NOT contain edges of the cycle
            dfs_find_segments(graph, node_id, visited, nodes, cycle, edges);
            segments.push_back(build_segment(nodes, edges, cycle));
        }
    }
}

Segment build_chord(
    const int attachment_1,
    const int attachment_2,
    const Cycle& cycle) {
    Segment chord;
    for (const int node_id : cycle)
        chord.get_segment().add_node(node_id);
    add_cycle_edges(cycle, chord);
    // adding chord edge
    chord.get_segment().add_edge(attachment_1, attachment_2);
    chord.add_attachment(attachment_1);
    chord.add_attachment(attachment_2);
    return chord;
}

void find_chords(
    const UndirectedGraph& graph,
    const Cycle& cycle,
    std::vector<Segment>& segments) {
    for (const int node_id : cycle) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (node_id < neighbor_id)
                continue;
            if (cycle.has_node(neighbor_id))
                if (neighbor_id != cycle.prev_of_node(node_id) &&
                    neighbor_id != cycle.next_of_node(node_id)) {
                    segments.push_back(
                        build_chord(node_id, neighbor_id, cycle));
                }
        }
    }
}

std::vector<Segment> compute_segments(
    const UndirectedGraph& graph,
    const Cycle& cycle) {
    std::vector<Segment> segments;
    find_segments(graph, cycle, segments);
    find_chords(graph, cycle, segments);
    return segments;
}