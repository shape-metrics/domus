#include "domus/planarity/auslander_parter.hpp"

#include <algorithm>
#include <deque>
#include <ranges>
#include <stddef.h>
#include <utility>
#include <vector>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "../core/domus_debug.hpp"
#include "domus/planarity/embedding.hpp"
#include "interlacement.hpp"
#include "segment.hpp"

/**
 * @brief if every biconnected component is embedded planarly, then merging them is straight forward
 *
 * @param graph
 * @param biconnected_components
 * @param embeddings
 * @return Embedding
 */
Embedding merge_biconnected_components(
    const Graph& graph,
    const BiconnectedComponents& biconnected_components,
    const std::vector<Embedding>& embeddings
) {
    Embedding output(graph);
    for (size_t i = 0; i < biconnected_components.get_components().size(); ++i) {
        const Embedding& embedding = embeddings[i];
        const Graph& component = biconnected_components.get_components()[i];
        const NodesLabels& labels = biconnected_components.get_labels_of_component(i);
        component.for_each_node([&](size_t node_id) {
            size_t old_node_id = labels.get_label(node_id);
            embedding.for_each_neighbor(node_id, [&](size_t component_neighbor_id) {
                size_t old_neighbor_id = labels.get_label(component_neighbor_id);
                output.add_edge(old_node_id, old_neighbor_id);
            });
        });
    }
    return output;
}

/**
 * @brief this function embeds a graph for which any embedding is planar (cycles for example)
 *
 * @param graph
 * @return An embedding of the graph
 */
Embedding base_case_graph(const Graph& graph) {
    Embedding embedding(graph);
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            embedding.add_edge(node_id, neighbor_id);
        });
    });
    DOMUS_ASSERT(is_embedding_planar(embedding), "base_case_graph: output embedding is not planar");
    return embedding;
}

/**
 * @brief this functions embeds a graph which consists of a cycle and a path attached to the cycle
 *
 * @param component
 * @param cycle
 * @return Embedding
 */
Embedding base_case_component(const Graph& component, const Cycle& cycle) {
    Embedding embedding(component);
    component.for_each_node([&](size_t node_id) {
        if (component.get_degree_of_node(node_id) == 2) {
            component.for_each_neighbor(node_id, [&](size_t neighbor_id) {
                embedding.add_edge(node_id, neighbor_id);
            });
            return;
        }
        DOMUS_ASSERT(
            component.get_degree_of_node(node_id) == 3,
            "base_case_component: node has degree different from 3"
        );
        DOMUS_ASSERT(
            cycle.has_node(node_id),
            "base_case_component: cycle does not contain the node with degree 3"
        );
        std::optional<size_t> neighbor_in_between;
        size_t position = cycle.node_position(node_id);
        component.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (neighbor_in_between.has_value())
                return;
            if (cycle.at(position + 1) == neighbor_id ||
                cycle.at(position + cycle.size() - 1) == neighbor_id)
                return;
            neighbor_in_between = neighbor_id;
        });
        embedding.add_edge(node_id, cycle.at(position + 1));
        embedding.add_edge(node_id, neighbor_in_between.value());
        embedding.add_edge(node_id, cycle.at(position + cycle.size() - 1));
    });
    DOMUS_ASSERT(
        is_embedding_planar(embedding),
        "base_case_component: output embedding is not planar"
    );
    return embedding;
}

Cycle change_cycle_with_path(
    const Cycle& cycle, const std::deque<size_t>& path, const std::optional<size_t> node_to_include
) {
    std::deque<size_t> nodes_copy(path); // newCycleList
    size_t first_of_path = path.front();
    size_t last_of_path = path.back();
    size_t position = cycle.node_position(last_of_path);
    size_t current = cycle.at(++position);
    bool foundNodeToInclude = !node_to_include.has_value();
    while (current != first_of_path) {
        nodes_copy.push_back(current);
        if (!foundNodeToInclude && current == *node_to_include)
            foundNodeToInclude = true;
        current = cycle.at(++position);
    }
    if (!foundNodeToInclude) {
        const std::deque reversed_path(path.rbegin(), path.rend());
        return change_cycle_with_path(cycle, reversed_path, node_to_include);
    }
    return Cycle(nodes_copy);
}

Cycle make_cycle_good(const Cycle& cycle, const Segment& segment) {
    std::vector<size_t> attachments_to_use;
    for (size_t i = 0; i < cycle.size(); ++i) {
        if (attachments_to_use.size() == 3)
            break;
        if (!segment.is_attachment(i))
            continue;
        attachments_to_use.push_back(i);
    }
    const std::deque<size_t> path =
        compute_path_between_attachments(segment, attachments_to_use[0], attachments_to_use[1]);
    auto& labels = segment.get_new_id_to_old_id();
    const std::deque<size_t> path_old_ids =
        path | std::views::transform([&](size_t node_id) { return labels.get_label(node_id); }) |
        std::ranges::to<std::deque<size_t>>();
    if (attachments_to_use.size() == 3)
        return change_cycle_with_path(cycle, path_old_ids, labels.get_label(attachments_to_use[2]));
    return change_cycle_with_path(cycle, path_old_ids, std::nullopt);
}

auto compute_min_and_max_segments_attachments(
    const std::vector<Segment>& segments, const Cycle& cycle
) {
    std::vector<size_t> segments_min_attachment(segments.size());
    std::vector<size_t> segments_max_attachment(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        size_t min = cycle.size();
        size_t max = 0;
        for (size_t node_id = 0; node_id < cycle.size(); ++node_id) {
            if (!segments[i].is_attachment(node_id))
                continue;
            min = std::min(min, node_id);
            max = std::max(max, node_id);
        }
        segments_min_attachment[i] = min;
        segments_max_attachment[i] = max;
    }
    return make_pair(segments_min_attachment, segments_max_attachment);
}

// true if, when drawing the cycle clockwise, the segment is inside it
std::vector<bool> are_embeddings_inside_clockwise_cycle(
    const Cycle& cycle,
    const std::vector<Embedding>& embeddings,
    const std::vector<Segment>& segments
) {
    std::vector<bool> is_inside(segments.size());
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        const Embedding& embedding = embeddings[i];
        size_t attachment_id = cycle.size(); // we want one attachment, anyone is good
        for (size_t node_id = 0; node_id < cycle.size(); ++node_id) {
            if (!segment.is_attachment(node_id))
                continue;
            attachment_id = node_id;
            break;
        }
        DOMUS_ASSERT(
            attachment_id != cycle.size(),
            "are_embeddings_inside_clockwise_cycle: no attachment found"
        );
        size_t next = (attachment_id + 1) % cycle.size();
        size_t prev = (attachment_id + cycle.size() - 1) % cycle.size();
        DOMUS_ASSERT(
            0 <= prev && prev < cycle.size(),
            "are_embeddings_inside_clockwise_cycle: prev should belong to the cycle"
        );
        is_inside[i] = embedding.next_element_in_adjacency_list(attachment_id, next) != prev;
    }
    return is_inside;
}

void compute_sub_order(
    std::vector<size_t>& sub_segments,
    const std::vector<size_t>& segments_attachment_index,
    const std::vector<Segment>& segments,
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
            const size_t num_attachments_first = segments[first_index].number_of_attachments();
            const size_t num_attachments_candidate =
                segments[candidate_index].number_of_attachments();
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

std::vector<size_t> compute_order(
    const std::vector<size_t>& segments_indexes,
    const std::vector<size_t>& segments_min_attachment,
    const std::vector<size_t>& segments_max_attachment,
    const std::vector<Segment>& segments,
    const size_t cycle_node_position
) {
    if (segments_indexes.size() < 2)
        return segments_indexes;
    std::optional<size_t> middle_segment{};
    std::vector<size_t> min_segments{};
    std::vector<size_t> max_segments{};
    for (size_t seg_index : segments_indexes) {
        if (segments_min_attachment[seg_index] == cycle_node_position) {
            min_segments.push_back(seg_index);
            continue;
        }
        if (segments_max_attachment[seg_index] == cycle_node_position) {
            max_segments.push_back(seg_index);
            continue;
        }
        DOMUS_ASSERT(!middle_segment.has_value(), "compute_order: internal errors");
        middle_segment = seg_index;
    }
    compute_sub_order(max_segments, segments_min_attachment, segments, false);
    compute_sub_order(min_segments, segments_max_attachment, segments, true);
    std::vector<size_t> order;
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
    const size_t cycle_node_position,
    const bool compatible,
    Embedding& output,
    const Cycle& cycle,
    const NodesLabels& labels_to_old_ids
) {
    const size_t prev_cycle_position = (cycle_node_position + cycle.size() - 1) % cycle.size();
    const size_t next_cycle_position = (cycle_node_position + 1) % cycle.size();
    std::vector<size_t> neighbors_to_add;
    size_t current = prev_cycle_position;
    for (size_t i = 1; i < embedding.get_node_degree(cycle_node_position); ++i) {
        current = embedding.next_element_in_adjacency_list(cycle_node_position, current);
        if (next_cycle_position == current || prev_cycle_position == current)
            continue;
        neighbors_to_add.push_back(current);
    }
    const size_t cycle_id = cycle[cycle_node_position];
    if (compatible)
        for (size_t neighbor_id : neighbors_to_add) {
            const size_t old_neighbor_id = labels_to_old_ids.get_label(neighbor_id);
            output.add_edge(cycle_id, old_neighbor_id);
        }
    else
        for (size_t j = neighbors_to_add.size(); j > 0; --j) {
            const size_t old_neighbor_id = labels_to_old_ids.get_label(neighbors_to_add[j - 1]);
            output.add_edge(cycle_id, old_neighbor_id);
        }
}

void add_edges_incident_to_cycle(
    const std::vector<Segment>& segments,
    const Cycle& cycle,
    const std::vector<Embedding>& embeddings,
    const Bipartition& is_segment_inside,
    Embedding& output,
    const std::vector<size_t>& segments_min_attachment,
    const std::vector<size_t>& segments_max_attachment,
    const std::vector<bool>& is_embedding_inside
) {
    for (size_t cycle_node_position = 0; cycle_node_position < cycle.size();
         ++cycle_node_position) {
        const size_t cycle_node_id = cycle[cycle_node_position];
        std::vector<size_t> inside_segments{};
        std::vector<size_t> outside_segments{};
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i].is_attachment(cycle_node_position)) {
                if (is_segment_inside.get_side(i))
                    inside_segments.push_back(i);
                else
                    outside_segments.push_back(i);
            }
        }
        // order of the segments inside the cycle
        std::vector<size_t> inside_order = compute_order(
            inside_segments,
            segments_min_attachment,
            segments_max_attachment,
            segments,
            cycle_node_position
        );
        std::ranges::reverse(inside_order);
        // order of the segments outside the cycle
        std::vector<size_t> outside_order = compute_order(
            outside_segments,
            segments_min_attachment,
            segments_max_attachment,
            segments,
            cycle_node_position
        );
        const size_t prev_cycle_node = cycle[cycle_node_position + cycle.size() - 1];
        const size_t next_cycle_node = cycle[cycle_node_position + 1];
        output.add_edge(cycle_node_id, next_cycle_node);
        for (const size_t segment_index : inside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const NodesLabels& labels_to_old_ids = segments[segment_index].get_new_id_to_old_id();
            const bool is_embedding_compatible =
                is_segment_inside.get_side(segment_index) == is_embedding_inside[segment_index];
            add_middle_edges(
                embedding,
                cycle_node_position,
                is_embedding_compatible,
                output,
                cycle,
                labels_to_old_ids
            );
        }
        output.add_edge(cycle_node_id, prev_cycle_node);
        for (const size_t segment_index : outside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const NodesLabels& labels_to_old_ids = segments[segment_index].get_new_id_to_old_id();
            const bool is_embedding_compatible =
                is_segment_inside.get_side(segment_index) == is_embedding_inside[segment_index];
            add_middle_edges(
                embedding,
                cycle_node_position,
                is_embedding_compatible,
                output,
                cycle,
                labels_to_old_ids
            );
        }
    }
}

void add_edges_not_incident_to_cycle(
    const std::vector<Segment>& segments,
    Embedding& output,
    const Cycle& cycle,
    const std::vector<Embedding>& embeddings,
    const std::vector<bool>& is_embedding_inside,
    const Bipartition& is_segment_inside
) {
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        const Embedding& embedding = embeddings[i];
        const NodesLabels& labels_to_old_ids = segment.get_new_id_to_old_id();
        segment.get_segment().for_each_node([&](size_t node_id) {
            if (node_id < cycle.size())
                return;
            const size_t old_node_id = labels_to_old_ids.get_label(node_id);
            std::vector<size_t> neighbors_to_add;
            embedding.for_each_neighbor(node_id, [&neighbors_to_add](size_t neighbor_id) {
                neighbors_to_add.push_back(neighbor_id);
            });
            if (is_segment_inside.get_side(i) == is_embedding_inside[i])
                for (size_t neighbor_id : neighbors_to_add) {
                    const size_t old_neighbor_id = labels_to_old_ids.get_label(neighbor_id);
                    output.add_edge(old_node_id, old_neighbor_id);
                }
            else
                for (size_t j = neighbors_to_add.size(); j > 0; --j) {
                    const size_t old_neighbor_id =
                        labels_to_old_ids.get_label(neighbors_to_add[j - 1]);
                    output.add_edge(old_node_id, old_neighbor_id);
                }
        });
    }
}

Embedding merge_segments_embeddings(
    const Graph& component,
    const Cycle& cycle,
    const std::vector<Embedding>& embeddings,
    const std::vector<Segment>& segments,
    const Bipartition& is_segment_inside
) {
    Embedding output(component);
    const auto [segments_min_attachment, segments_max_attachment] =
        compute_min_and_max_segments_attachments(segments, cycle);
    const std::vector<bool> is_embedding_inside =
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

std::optional<Embedding> embed_biconnected_component(const Graph& component);

bool is_embedding_valid(const Segment& segment, const Embedding& embedding) {
    const Graph& graph = segment.get_segment();
    if (graph.size() != embedding.size())
        return false;
    if (graph.get_number_of_edges() * 2 != embedding.total_number_of_edges())
        return false;
    bool is_valid = true;
    embedding.for_each_node([&](size_t node_id) {
        if (!is_valid)
            return;
        if (!graph.has_node(node_id))
            is_valid = false;
        embedding.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (!graph.are_neighbors(node_id, neighbor_id))
                is_valid = false;
        });
        if (embedding.get_node_degree(node_id) != graph.get_degree_of_node(node_id))
            is_valid = false;
    });
    if (!is_valid)
        return false;
    return is_embedding_planar(embedding);
}

std::optional<Embedding> embed_biconnected_component(const Graph& component, const Cycle& cycle) {
    const std::vector<Segment> segments = Segment::compute(component, cycle);
    if (segments.empty()) // the entire biconnected component is a cycle
        return base_case_graph(component);
    if (segments.size() == 1) {
        const Segment& segment = segments[0];
        if (is_segment_a_path(segment))
            return base_case_component(component, cycle);
        // the chosen cycle is bad
        return embed_biconnected_component(component, make_cycle_good(cycle, segment));
    }
    const Graph interlacement_graph = compute_interlacement_graph(segments, cycle);
    const std::optional<Bipartition> is_segment_inside = compute_bipartition(interlacement_graph);
    if (!is_segment_inside.has_value())
        return std::nullopt; // if no bipartition exists, the component is not planar
    std::vector<Embedding> embeddings;
    for (const Segment& segment : segments) {
        std::optional<Embedding> embedding = embed_biconnected_component(segment.get_segment());
        if (!embedding.has_value())
            return std::nullopt;
        embeddings.push_back(std::move(embedding.value()));
        DOMUS_ASSERT(
            is_embedding_valid(segment, embeddings.back()),
            "embed_biconnected_component: invalid embedding"
        );
    }
    return merge_segments_embeddings(
        component,
        cycle,
        embeddings,
        segments,
        is_segment_inside.value()
    );
}

std::optional<Embedding> embed_biconnected_component(const Graph& component) {
    const std::optional<Cycle> cycle = find_an_undirected_cycle_in_graph(component);
    if (cycle.has_value())
        return embed_biconnected_component(component, cycle.value());
    return base_case_graph(component);
}

std::optional<Embedding> embed_graph(const Graph& graph) {
    if (graph.size() < 4)
        return base_case_graph(graph);
    if (graph.get_number_of_edges() / 2 > 3 * graph.size() - 6)
        return std::nullopt;
    const BiconnectedComponents bic_comps = compute_biconnected_components(graph);
    std::vector<Embedding> embeddings;
    for (const auto& component : bic_comps.get_components()) {
        std::optional<Embedding> embedding = embed_biconnected_component(component);
        if (!embedding.has_value())
            return std::nullopt;
        embeddings.push_back(std::move(embedding.value()));
    }
    return merge_biconnected_components(graph, bic_comps, embeddings);
}