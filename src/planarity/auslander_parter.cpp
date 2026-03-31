#include "domus/planarity/auslander_parter.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "domus/core/graph/concept.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "domus/core/domus_debug.hpp"
#include "interlacement.hpp"
#include "segment.hpp"

namespace domus::planarity {
using namespace domus::graph::algorithms;
using namespace domus::graph::utilities;

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
        for (size_t node_id : component.get_nodes_ids()) {
            size_t old_node_id = labels.get_label(node_id);
            for (const EdgeIter component_edge : embedding.get_edges(node_id)) {
                size_t old_neighbor_id = labels.get_label(component_edge.neighbor_id);
                output.add_edge(old_node_id, old_neighbor_id, component_edge.id);
            }
        }
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
    for (size_t node_id : graph.get_nodes_ids())
        for (EdgeIter edge : graph.get_edges(node_id))
            embedding.add_edge(node_id, edge.neighbor_id, edge.id);
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
    for (const size_t node_id : component.get_nodes_ids()) {
        if (component.get_degree_of_node(node_id) == 2) {
            for (const EdgeIter edge : component.get_edges(node_id))
                embedding.add_edge(node_id, edge.neighbor_id, edge.id);
            continue;
        }
        DOMUS_ASSERT(
            component.get_degree_of_node(node_id) == 3,
            "base_case_component: node has degree different from 3"
        );
        DOMUS_ASSERT(
            cycle.has_node_id(node_id),
            "base_case_component: cycle does not contain the node with degree 3"
        );
        std::optional<EdgeIter> edge_in_between;
        size_t position = cycle.node_id_position(node_id);
        for (const EdgeIter edge : component.get_edges(node_id)) {
            if (cycle.node_id_at(position + 1) == edge.neighbor_id ||
                cycle.node_id_at(position + cycle.size() - 1) == edge.neighbor_id)
                continue;
            edge_in_between = edge;
            break;
        }
        embedding.add_edge(node_id, cycle.node_id_at(position + 1), cycle.edge_id_at(position));
        embedding.add_edge(node_id, edge_in_between->neighbor_id, edge_in_between->id);
        embedding.add_edge(
            node_id,
            cycle.node_id_at(position + cycle.size() - 1),
            cycle.edge_id_at(position + cycle.size() - 1)
        );
    }
    DOMUS_ASSERT(
        is_embedding_planar(embedding),
        "base_case_component: output embedding is not planar"
    );
    return embedding;
}

Cycle change_cycle_with_path(
    const Graph& graph, const Cycle& cycle, Path& path, const std::optional<size_t> node_to_include
) {
    Path path_copy(path); // newCycleList
    size_t first_of_path = path.get_first_node_id();
    size_t last_of_path = path.get_last_node_id();
    size_t position = cycle.node_id_position(last_of_path);
    size_t curr_node_id = cycle.node_id_at(position);
    size_t curr_edge_id = cycle.edge_id_at(position);
    bool foundNodeToInclude = !node_to_include.has_value();
    while (curr_node_id != first_of_path) {
        path_copy.push_back(graph, curr_node_id, curr_edge_id);
        if (!foundNodeToInclude && curr_node_id == *node_to_include)
            foundNodeToInclude = true;
        curr_node_id = cycle.node_id_at(++position);
        curr_edge_id = cycle.edge_id_at(position);
    }
    if (!foundNodeToInclude) {
        path.reverse();
        return change_cycle_with_path(graph, cycle, path, node_to_include);
    }
    return Cycle(path_copy);
}

Cycle make_cycle_good(const Graph& graph, const Cycle& cycle, const Segment& segment) {
    std::vector<size_t> attachments_to_use;
    for (size_t i = 0; i < cycle.size(); ++i) {
        if (attachments_to_use.size() == 3)
            break;
        if (!segment.is_attachment(i))
            continue;
        attachments_to_use.push_back(i);
    }
    const Path path =
        compute_path_between_attachments(segment, attachments_to_use[0], attachments_to_use[1]);
    auto& nodes_labels = segment.get_new_id_to_old_id();
    auto& edge_labels = segment.get_new_edge_id_to_old_id();
    Path old_path;
    path.for_each([&](size_t edge_id, size_t prev_node_id) {
        size_t old_edge_id = edge_labels.get_label(edge_id);
        size_t old_prev_node_id = nodes_labels.get_label(prev_node_id);
        old_path.push_back(graph, old_prev_node_id, old_edge_id);
    });
    if (attachments_to_use.size() == 3) {
        return change_cycle_with_path(
            graph,
            cycle,
            old_path,
            nodes_labels.get_label(attachments_to_use[2])
        );
    }
    return change_cycle_with_path(graph, cycle, old_path, std::nullopt);
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
        is_inside[i] =
            embedding.next_in_adjacency_list(attachment_id, next, attachment_id).neighbor_id !=
            prev;
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
    const NodesLabels& labels_to_old_ids,
    const EdgesLabels& to_old_edge_ids
) {
    const size_t prev_cycle_position = (cycle_node_position + cycle.size() - 1) % cycle.size();
    const size_t next_cycle_position = (cycle_node_position + 1) % cycle.size();
    std::vector<EdgeIter> edges_to_add;
    size_t current = prev_cycle_position;
    size_t current_edge_id = prev_cycle_position;
    for (size_t i = 0; i < embedding.get_degree_of_node(cycle_node_position); ++i) {
        auto edge = embedding.next_in_adjacency_list(cycle_node_position, current, current_edge_id);
        current = edge.neighbor_id;
        current_edge_id = edge.id;
        if (next_cycle_position == edge.neighbor_id || prev_cycle_position == edge.neighbor_id)
            continue;
        edges_to_add.push_back(edge);
    }
    const size_t cycle_id = cycle.node_id_at(cycle_node_position);
    if (compatible)
        for (EdgeIter edge : edges_to_add) {
            const size_t old_neighbor_id = labels_to_old_ids.get_label(edge.neighbor_id);
            const size_t old_edge_id = to_old_edge_ids.get_label(edge.id);
            output.add_edge(cycle_id, old_neighbor_id, old_edge_id);
        }
    else
        for (size_t j = edges_to_add.size(); j > 0; --j) {
            EdgeIter edge = edges_to_add[j - 1];
            const size_t old_neighbor_id = labels_to_old_ids.get_label(edge.neighbor_id);
            const size_t old_edge_id = to_old_edge_ids.get_label(edge.id);
            output.add_edge(cycle_id, old_neighbor_id, old_edge_id);
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
        const size_t cycle_node_id = cycle.node_id_at(cycle_node_position);
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
        const size_t prev_cycle_node = cycle.node_id_at(cycle_node_position + cycle.size() - 1);
        const size_t next_cycle_node = cycle.node_id_at(cycle_node_position + 1);
        output.add_edge(cycle_node_id, next_cycle_node, cycle.edge_id_at(cycle_node_position));
        for (const size_t segment_index : inside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const NodesLabels& labels_to_old_ids = segments[segment_index].get_new_id_to_old_id();
            const EdgesLabels& labels_to_old_edge_ids =
                segments[segment_index].get_new_edge_id_to_old_id();
            const bool is_embedding_compatible =
                is_segment_inside.get_side(segment_index) == is_embedding_inside[segment_index];
            add_middle_edges(
                embedding,
                cycle_node_position,
                is_embedding_compatible,
                output,
                cycle,
                labels_to_old_ids,
                labels_to_old_edge_ids
            );
        }
        output.add_edge(
            cycle_node_id,
            prev_cycle_node,
            cycle.edge_id_at(cycle_node_position + cycle.size() - 1)
        );
        for (const size_t segment_index : outside_order) {
            const Embedding& embedding = embeddings[segment_index];
            const NodesLabels& labels_to_old_ids = segments[segment_index].get_new_id_to_old_id();
            const EdgesLabels& labels_to_old_edge_ids =
                segments[segment_index].get_new_edge_id_to_old_id();
            const bool is_embedding_compatible =
                is_segment_inside.get_side(segment_index) == is_embedding_inside[segment_index];
            add_middle_edges(
                embedding,
                cycle_node_position,
                is_embedding_compatible,
                output,
                cycle,
                labels_to_old_ids,
                labels_to_old_edge_ids
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
        const EdgesLabels& labels_to_old_edge_ids = segment.get_new_edge_id_to_old_id();
        for (const size_t node_id : segment.get_segment().get_nodes_ids()) {
            if (node_id < cycle.size())
                continue;
            const size_t old_node_id = labels_to_old_ids.get_label(node_id);
            std::vector<EdgeIter> edges_to_add =
                std::ranges::to<std::vector<EdgeIter>>(embedding.get_edges(node_id));
            if (is_segment_inside.get_side(i) == is_embedding_inside[i])
                for (EdgeIter edge : edges_to_add) {
                    const size_t old_neighbor_id = labels_to_old_ids.get_label(edge.neighbor_id);
                    const size_t old_edge_id = labels_to_old_edge_ids.get_label(edge.id);
                    output.add_edge(old_node_id, old_neighbor_id, old_edge_id);
                }
            else
                for (size_t j = edges_to_add.size(); j > 0; --j) {
                    const size_t old_neighbor_id =
                        labels_to_old_ids.get_label(edges_to_add[j - 1].neighbor_id);
                    const size_t old_edge_id =
                        labels_to_old_edge_ids.get_label(edges_to_add[j - 1].id);
                    output.add_edge(old_node_id, old_neighbor_id, old_edge_id);
                }
        }
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
    if (graph.get_number_of_nodes() != embedding.get_number_of_nodes())
        return false;
    if (graph.get_number_of_edges() * 2 != embedding.get_number_of_edges())
        return false;
    for (const size_t node_id : embedding.get_nodes_ids()) {
        if (!graph.has_node(node_id))
            return false;
        for (const size_t neighbor_id : embedding.get_neighbors(node_id))
            if (!graph.are_neighbors(node_id, neighbor_id))
                return false;
        if (embedding.get_degree_of_node(node_id) != graph.get_degree_of_node(node_id))
            return false;
    }
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
        const Cycle good_cycle = make_cycle_good(component, cycle, segment);
        DOMUS_ASSERT(
            Segment::compute(component, good_cycle).size() > 1,
            "embed_biconnected_component: make_cycle_good failed"
        );
        return embed_biconnected_component(component, good_cycle);
    }
    const Graph interlacement_graph = compute_interlacement_graph(segments, cycle);
    const std::optional<Bipartition> is_segment_inside = Bipartition::compute(interlacement_graph);
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

std::optional<Embedding> compute_planar_embedding(const Graph& graph) {
    if (graph.get_number_of_nodes() < 4)
        return base_case_graph(graph);
    if (graph.get_number_of_edges() / 2 > 3 * graph.get_number_of_nodes() - 6)
        return std::nullopt;
    const BiconnectedComponents bic_comps = BiconnectedComponents::compute(graph);
    std::vector<Embedding> embeddings;
    for (const auto& component : bic_comps.get_components()) {
        std::optional<Embedding> embedding = embed_biconnected_component(component);
        if (!embedding.has_value())
            return std::nullopt;
        embeddings.push_back(std::move(embedding.value()));
    }
    Embedding result = merge_biconnected_components(graph, bic_comps, embeddings);
    DOMUS_ASSERT(
        is_embedding_planar(result),
        "compute_planar_embedding: output embedding is not planar"
    );
    return result;
}

bool is_biconnected_component_planar(const Graph& component);

bool is_graph_planar(const graph::Graph& graph) {
    if (graph.get_number_of_nodes() <= 4)
        return true;
    if (graph.get_number_of_edges() / 2 > 3 * graph.get_number_of_nodes() - 6)
        return false;
    for (const Graph& component : BiconnectedComponents::compute(graph).get_components())
        if (!is_biconnected_component_planar(component))
            return false;
    return true;
}

bool is_biconnected_component_planar(const Graph& component, const Cycle& cycle) {
    const std::vector<Segment> segments = Segment::compute(component, cycle);
    if (segments.empty()) // the entire biconnected component is a cycle
        return true;
    if (segments.size() == 1) {
        const Segment& segment = segments[0];
        if (is_segment_a_path(segment))
            return true;
        // the chosen cycle is bad
        const Cycle good_cycle = make_cycle_good(component, cycle, segment);
        DOMUS_ASSERT(
            Segment::compute(component, good_cycle).size() > 1,
            "embed_biconnected_component: make_cycle_good failed"
        );
        return is_biconnected_component_planar(component, good_cycle);
    }
    const Graph interlacement_graph = compute_interlacement_graph(segments, cycle);
    if (!Bipartition::is_bipartite(interlacement_graph))
        return false; // if no bipartition exists, the component is not planar
    for (const Segment& segment : segments)
        if (!is_biconnected_component_planar(segment.get_segment()))
            return false;
    return true;
}

bool is_biconnected_component_planar(const Graph& component) {
    const std::optional<Cycle> cycle = find_an_undirected_cycle_in_graph(component);
    if (cycle.has_value())
        return is_biconnected_component_planar(component, cycle.value());
    return true;
}

} // namespace domus::planarity