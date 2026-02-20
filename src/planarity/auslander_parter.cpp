#include "domus/planarity/auslander_parter.hpp"

#include <algorithm>
#include <deque>
#include <iostream>
#include <stddef.h>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/segment.hpp"
#include "domus/core/utils.hpp"
#include "domus/planarity/interlacement.hpp"

using namespace std;

Embedding merge_biconnected_components(
    const UndirectedGraph& graph,
    const BiconnectedComponents& biconnected_components,
    const vector<Embedding>& embeddings
) {
    Embedding output(graph);
    for (size_t i = 0; i < biconnected_components.get_components().size(); ++i) {
        const Embedding& embedding = embeddings[i];
        const UndirectedGraph& component = biconnected_components.get_components()[i];
        for (const int node_id : component.get_nodes_ids()) {
            for (const int component_neighbor_id : embedding.get_adjacency_list(node_id))
                output.add_edge(node_id, component_neighbor_id);
        }
    }
    return output;
}

Embedding base_case_graph(const UndirectedGraph& graph) {
    Embedding embedding(graph);
    for (int node_id : graph.get_nodes_ids())
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            embedding.add_edge(node_id, neighbor_id);
    return embedding;
}

Embedding base_case_component(const UndirectedGraph& component, const Cycle& cycle) {
    Embedding embedding(component);
    for (int node_id : component.get_nodes_ids()) {
        if (component.get_degree_of_node(node_id) == 2) {
            for (int neighbor_id : component.get_neighbors_of_node(node_id))
                embedding.add_edge(node_id, neighbor_id);
            continue;
        }
        optional<int> neighbor_in_between;
        for (int neighbor_id : component.get_neighbors_of_node(node_id)) {
            if (cycle.next_of_node(node_id) == neighbor_id ||
                cycle.prev_of_node(node_id) == neighbor_id) {
                continue;
            }
            neighbor_in_between = neighbor_id;
            break;
        }
        embedding.add_edge(node_id, cycle.next_of_node(node_id));
        embedding.add_edge(node_id, neighbor_in_between.value());
        embedding.add_edge(node_id, cycle.prev_of_node(node_id));
    }
    return embedding;
}

Cycle change_cycle_with_path(
    const Cycle& cycle, const deque<int>& path, const optional<int> node_to_include
) {
    deque<int> nodes_copy(path); // newCycleList
    const int first_of_path = path.front();
    const int last_of_path = path.back();
    int current = cycle.next_of_node(last_of_path);
    bool foundNodeToInclude = !node_to_include.has_value();
    while (current != first_of_path) {
        nodes_copy.push_back(current);
        if (!foundNodeToInclude && current == *node_to_include)
            foundNodeToInclude = true;
        current = cycle.next_of_node(current);
    }
    if (!foundNodeToInclude) {
        const deque reversed_path(path.rbegin(), path.rend());
        return change_cycle_with_path(cycle, reversed_path, node_to_include);
    }
    return Cycle(nodes_copy);
}

Cycle make_cycle_good(const Cycle& cycle, const Segment& segment) {
    vector<int> attachments_to_use{};
    for (const int cycle_node_id : cycle) {
        if (!segment.has_attachment(cycle_node_id))
            continue;
        attachments_to_use.push_back(cycle_node_id);
        if (attachments_to_use.size() == 3)
            break;
    }
    const deque<int> path =
        compute_path_between_attachments(segment, attachments_to_use[0], attachments_to_use[1]);
    if (attachments_to_use.size() == 3)
        return change_cycle_with_path(cycle, path, attachments_to_use[2]);
    return change_cycle_with_path(cycle, path, std::nullopt);
}

auto compute_min_and_max_segments_attachments(const vector<Segment>& segments, const Cycle& cycle) {
    vector<size_t> segments_min_attachment(segments.size());
    vector<size_t> segments_max_attachment(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        size_t min = cycle.size();
        size_t max = 0;
        for (const int attachment : segments[i].get_attachments()) {
            const size_t position = cycle.node_position(attachment);
            min = std::min(min, position);
            max = std::max(max, position);
        }
        segments_min_attachment[i] = min;
        segments_max_attachment[i] = max;
    }
    return make_tuple(segments_min_attachment, segments_max_attachment);
}

// true if, when drawing the cycle clockwise, the segment is inside it
vector<bool> are_embeddings_inside_clockwise_cycle(
    const Cycle& cycle, const vector<Embedding>& embeddings, const vector<Segment>& segments
) {
    vector<bool> is_inside(segments.size());
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        const Embedding& embedding = embeddings[i];
        const int attachment_id = *segment.get_attachments().begin(); // any attachment is good
        const int next = cycle.next_of_node(attachment_id);
        const int prev = cycle.prev_of_node(attachment_id);
        is_inside[i] = embedding.get_adjacency_list(attachment_id).next_element(next) != prev;
    }
    return is_inside;
}

void compute_sub_order(
    vector<size_t>& sub_segments,
    const vector<size_t>& segments_attachment_index,
    const vector<Segment>& segments,
    const bool ordering_min_segments
) {
    if (sub_segments.size() < 2)
        return;
    for (size_t i = 0; i < sub_segments.size() - 1; ++i) {
        size_t first = i;
        size_t first_index = sub_segments[i];
        for (size_t j = i + 1; j < sub_segments.size(); ++j) {
            const size_t candidate_index = sub_segments[j];
            if (segments_attachment_index[candidate_index] < segments_attachment_index[first_index])
                continue;
            if (segments_attachment_index[candidate_index] >
                segments_attachment_index[first_index]) {
                first_index = candidate_index;
                first = j;
                continue;
            }
            const size_t num_attachments_first = segments[first_index].get_attachments().size();
            const size_t num_attachments_candidate =
                segments[candidate_index].get_attachments().size();
            if (num_attachments_first == num_attachments_candidate) {
                if (first_index > candidate_index)
                    continue;
                first_index = candidate_index;
                first = j;
                continue;
            }
            if (ordering_min_segments == (num_attachments_candidate == 2)) {
                first_index = candidate_index;
                first = j;
            }
        }
        const size_t temp = sub_segments[first];
        sub_segments[first] = sub_segments[i];
        sub_segments[i] = temp;
    }
}

vector<size_t> compute_order(
    const vector<size_t>& segments_indexes,
    const vector<size_t>& segments_min_attachment,
    const vector<size_t>& segments_max_attachment,
    const vector<Segment>& segments,
    const size_t cycle_node_position
) {
    if (segments_indexes.size() < 2)
        return segments_indexes;
    optional<size_t> middle_segment = std::nullopt;
    vector<size_t> min_segments{};
    vector<size_t> max_segments{};
    for (size_t seg_index : segments_indexes) {
        if (segments_min_attachment[seg_index] == cycle_node_position) {
            min_segments.push_back(seg_index);
            continue;
        }
        if (segments_max_attachment[seg_index] == cycle_node_position) {
            max_segments.push_back(seg_index);
            continue;
        }
        if (middle_segment.has_value())
            throw runtime_error("More than one");
        middle_segment = seg_index;
    }
    compute_sub_order(max_segments, segments_min_attachment, segments, false);
    compute_sub_order(min_segments, segments_max_attachment, segments, true);
    vector<size_t> order;
    for (size_t segment_index : max_segments)
        order.push_back(segment_index);
    if (middle_segment.has_value())
        order.push_back(middle_segment.value());
    for (size_t segment_index : min_segments)
        order.push_back(segment_index);
    return order;
}

void add_middle_edges(
    const Embedding& embedding,
    const int cycle_node_id,
    const bool compatible,
    Embedding& output,
    const Cycle& cycle
) {
    const int prev_cycle_node_id = cycle.prev_of_node(cycle_node_id);
    const int next_cycle_node_id = cycle.next_of_node(cycle_node_id);
    vector<int> neighbors_to_add;
    int current = prev_cycle_node_id;
    for (size_t i = 1; i < embedding.get_adjacency_list(cycle_node_id).size(); ++i) {
        current = embedding.get_adjacency_list(cycle_node_id).next_element(current);
        if (next_cycle_node_id == current)
            continue;
        if (prev_cycle_node_id == current)
            continue;
        neighbors_to_add.push_back(current);
    }
    if (compatible)
        for (const int neighbor_id : neighbors_to_add)
            output.add_edge(cycle_node_id, neighbor_id);
    else
        for (size_t j = neighbors_to_add.size(); j > 0; --j)
            output.add_edge(cycle_node_id, neighbors_to_add[j - 1]);
}

void add_edges_incident_to_cycle(
    const vector<Segment>& segments,
    const Cycle& cycle,
    const vector<Embedding>& embeddings,
    const unordered_map<int, bool>& is_segment_inside,
    Embedding& output,
    const vector<size_t>& segments_min_attachment,
    const vector<size_t>& segments_max_attachment,
    const vector<bool>& is_embedding_inside
) {
    for (size_t cycle_node_position = 0; cycle_node_position < cycle.size();
         ++cycle_node_position) {
        const int cycle_node_id = cycle[cycle_node_position];
        vector<size_t> inside_segments{};
        vector<size_t> outside_segments{};
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i].has_attachment(cycle_node_id)) {
                if (is_segment_inside.at(static_cast<int>(i)))
                    inside_segments.push_back(i);
                else
                    outside_segments.push_back(i);
            }
        }
        // order of the segments inside the cycle
        vector<size_t> inside_order = compute_order(
            inside_segments,
            segments_min_attachment,
            segments_max_attachment,
            segments,
            cycle_node_position
        );
        ranges::reverse(inside_order);
        // order of the segments outside the cycle
        vector<size_t> outside_order = compute_order(
            outside_segments,
            segments_min_attachment,
            segments_max_attachment,
            segments,
            cycle_node_position
        );
        const int prev_cycle_node = cycle.prev_of_node(cycle_node_id);
        const int next_cycle_node = cycle.next_of_node(cycle_node_id);
        output.add_edge(cycle_node_id, next_cycle_node);
        for (const size_t segment_index : inside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const bool is_embedding_compatible =
                is_segment_inside.at(static_cast<int>(segment_index)) ==
                is_embedding_inside[segment_index];
            add_middle_edges(embedding, cycle_node_id, is_embedding_compatible, output, cycle);
        }
        output.add_edge(cycle_node_id, prev_cycle_node);
        for (const size_t segment_index : outside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const bool is_embedding_compatible =
                is_segment_inside.at(static_cast<int>(segment_index)) ==
                is_embedding_inside[segment_index];
            add_middle_edges(embedding, cycle_node_id, is_embedding_compatible, output, cycle);
        }
    }
}

void add_edges_not_incident_to_cycle(
    const vector<Segment>& segments,
    Embedding& output,
    const Cycle& cycle,
    const vector<Embedding>& embeddings,
    const vector<bool>& is_embedding_inside,
    const unordered_map<int, bool>& is_segment_inside
) {
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        const Embedding& embedding = embeddings[i];
        for (const int node_id : segment.get_segment().get_nodes_ids()) {
            if (cycle.has_node(node_id))
                continue;
            vector<int> neighbors_to_add;
            for (const int neighbor_id : embedding.get_adjacency_list(node_id))
                neighbors_to_add.push_back(neighbor_id);
            if (is_segment_inside.at(static_cast<int>(i)) == is_embedding_inside[i])
                for (const int neighbor_id : neighbors_to_add)
                    output.add_edge(node_id, neighbor_id);
            else
                for (size_t j = neighbors_to_add.size(); j > 0; --j)
                    output.add_edge(node_id, neighbors_to_add[j - 1]);
        }
    }
}

Embedding merge_segments_embeddings(
    const UndirectedGraph& component,
    const Cycle& cycle,
    const vector<Embedding>& embeddings,
    const vector<Segment>& segments,
    const unordered_map<int, bool>& is_segment_inside
) {
    Embedding output(component);
    const auto [segments_min_attachment, segments_max_attachment] =
        compute_min_and_max_segments_attachments(segments, cycle);
    const vector<bool> is_embedding_inside =
        are_embeddings_inside_clockwise_cycle(cycle, embeddings, segments);
    add_edges_incident_to_cycle(
        segments,
        cycle,
        embeddings,
        is_segment_inside,
        output,
        segments_min_attachment,
        segments_max_attachment,
        is_embedding_inside
    );
    add_edges_not_incident_to_cycle(
        segments,
        output,
        cycle,
        embeddings,
        is_embedding_inside,
        is_segment_inside
    );
    return output;
}

optional<Embedding> embed_biconnected_component(const UndirectedGraph& component);

optional<Embedding>
embed_biconnected_component(const UndirectedGraph& component, const Cycle& cycle) {
    const vector<Segment> segments = compute_segments(component, cycle);
    if (segments.empty()) // the entire biconnected component is a cycle
        return base_case_graph(component);
    if (segments.size() == 1) {
        const Segment& segment = segments[0];
        if (is_segment_a_path(segment))
            return base_case_component(component, cycle);
        // the chosen cycle is bad
        Cycle new_cycle = make_cycle_good(cycle, segment);
        new_cycle.print();
        return embed_biconnected_component(component, new_cycle);
    }
    const UndirectedGraph interlacement_graph = compute_interlacement_graph(segments, cycle);
    const optional<unordered_map<int, bool>> is_segment_inside =
        compute_bipartition(interlacement_graph);
    if (!is_segment_inside.has_value())
        return std::nullopt; // if no bipartition exists, the component is not planar
    vector<Embedding> embeddings;
    for (const Segment& segment : segments) {
        optional<Embedding> embedding = embed_biconnected_component(segment.get_segment());
        if (!embedding.has_value())
            return std::nullopt;
        embeddings.push_back(std::move(embedding.value()));
    }
    std::cout << "embedded biconnected component\n";
    return merge_segments_embeddings(
        component,
        cycle,
        embeddings,
        segments,
        is_segment_inside.value()
    );
}

optional<Embedding> embed_biconnected_component(const UndirectedGraph& component) {
    const optional<Cycle> cycle = find_a_cycle_in_graph(component);
    if (cycle.has_value())
        return embed_biconnected_component(component, cycle.value());
    return base_case_graph(component);
}

optional<Embedding> embed_graph(const UndirectedGraph& graph) {
    if (graph.size() < 4)
        return base_case_graph(graph);
    if (graph.get_number_of_edges() / 2 > 3 * graph.size() - 6)
        return std::nullopt;
    const BiconnectedComponents bic_comps = compute_biconnected_components(graph);
    vector<Embedding> embeddings;
    for (const auto& component : bic_comps.get_components()) {
        optional<Embedding> embedding = embed_biconnected_component(component);
        if (!embedding.has_value())
            return std::nullopt;
        embeddings.push_back(std::move(embedding.value()));
    }
    return merge_biconnected_components(graph, bic_comps, embeddings);
}