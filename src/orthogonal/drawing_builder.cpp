#include "domus/orthogonal/drawing_builder.hpp"

#include <algorithm>
#include <limits.h>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/concept.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/orthogonal/area_compacter.hpp"
#include "domus/orthogonal/equivalence_classes.hpp"
#include "domus/orthogonal/shape/direction.hpp"
#include "domus/orthogonal/shape/shape.hpp"
#include "domus/orthogonal/shape/shape_builder.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::orthogonal {
using namespace domus::graph;
using shape::build_shape;
using shape::Direction;
using shape::Shape;

const Path path_in_class(
    const Graph& graph, size_t from_id, size_t to_id, const Shape& shape, bool go_horizontal
) {
    utilities::NodesLabels<size_t> parent(graph);
    std::stack<size_t> stack;
    stack.push(from_id);

    while (!stack.empty()) {
        size_t current = stack.top();
        stack.pop();
        if (parent.has_label(to_id))
            break;
        for (auto [edge_id, neighbor_id] : graph.get_edges(current)) {
            if (parent.has_label(neighbor_id))
                continue;
            if (go_horizontal == shape.is_horizontal(edge_id)) {
                parent.add_label(neighbor_id, edge_id);
                stack.push(neighbor_id);
            }
        }
    }

    Path path;
    size_t curr_node_id = to_id;
    while (curr_node_id != from_id) {
        size_t edge_id = parent.get_label(curr_node_id);
        path.push_front(graph, curr_node_id, edge_id);
        auto [f_id, t_id] = graph.get_edge(edge_id);
        curr_node_id = (f_id == curr_node_id) ? t_id : f_id;
    }
    return path;
}

inline std::pair<size_t, size_t>
get_other_edge_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    DOMUS_ASSERT(
        graph.get_degree_of_node(node_id) == 2,
        "get_other_neighbor_id: function only for degree 2 nodes"
    );
    std::optional<size_t> other;
    std::optional<size_t> other_edge_id;
    for (const EdgeIter& edge : graph.get_edges(node_id))
        if (edge.neighbor_id != neighbor_id) {
            other = edge.neighbor_id;
            other_edge_id = edge.id;
            break;
        }

    DOMUS_ASSERT(
        other.has_value(),
        "get_other_neighbor_id: internal error happened, no other neighbor found for node"
    );
    return {*other, *other_edge_id};
}

inline size_t get_other_neighbor_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    return get_other_edge_id(graph, node_id, neighbor_id).first;
}

Edge get_edge_in_graph(
    const Graph& graph,
    const size_t graph_edge_id,
    const size_t class_from,
    const size_t class_to,
    const EquivalenceClasses& classes
) {
    auto [from_id, to_id] = graph.get_edge(graph_edge_id);
    DOMUS_ASSERT(
        classes.get_class_of_elem(from_id) == class_from ||
            classes.get_class_of_elem(from_id) == class_to,
        "get_edge_in_graph: class doesnt correspond to edge"
    );
    DOMUS_ASSERT(
        classes.get_class_of_elem(to_id) == class_from ||
            classes.get_class_of_elem(to_id) == class_to,
        "get_edge_in_graph: class doesnt correspond to edge"
    );
    if (classes.get_class_of_elem(from_id) == class_from)
        return {from_id, to_id};
    return {to_id, from_id};
}

Cycle build_cycle_in_graph_from_cycle_in_ordering(
    const EquivalenceClasses& classes,
    const Graph& graph,
    const Shape& shape,
    const Cycle& cycle_in_ordering,
    const graph::utilities::EdgesLabels<size_t>& ordering_edge_to_graph_edge,
    bool go_horizontal
) {
    Path cycle;
    for (size_t i = 0; i < cycle_in_ordering.size(); ++i) {
        const size_t edge_id_ordering = cycle_in_ordering.edge_id_at(i);
        const size_t edge_id_graph = ordering_edge_to_graph_edge.get_label(edge_id_ordering);

        const size_t class_from = cycle_in_ordering.node_id_at(i);
        const size_t class_to = cycle_in_ordering.node_id_at(i + 1);

        const auto [from_id, to_id] =
            get_edge_in_graph(graph, edge_id_graph, class_from, class_to, classes);

        cycle.push_back(graph, from_id, edge_id_graph);

        const size_t next_edge_id_ordering = cycle_in_ordering.edge_id_at(i + 1);
        const size_t next_edge_id_graph =
            ordering_edge_to_graph_edge.get_label(next_edge_id_ordering);
        const size_t next_class_to = cycle_in_ordering.node_id_at(i + 2);
        const auto [next_from_id, next_to_id] =
            get_edge_in_graph(graph, next_edge_id_graph, class_to, next_class_to, classes);
        if (to_id != next_from_id) {
            const Path path = path_in_class(graph, to_id, next_from_id, shape, go_horizontal);
            DOMUS_ASSERT(
                path.number_of_edges() >= 1,
                "build_cycle_in_graph_from_cycle_in_ordering: found path is too small"
            );
            DOMUS_ASSERT(
                path.get_first_node_id() == to_id && path.get_last_node_id() == next_from_id,
                "build_cycle_in_graph_from_cycle_in_ordering: built path is not valid"
            );
            for (auto [edge_id, prev_node_id] : path.get_edges())
                cycle.push_back(graph, prev_node_id, edge_id);
        }
    }
    Cycle result(cycle);
    DOMUS_ASSERT(
        algorithms::is_cycle_in_graph(graph, result),
        "build_cycle_in_graph_from_cycle_in_ordering: built cycle is not valid"
    );
    return result;
}

// useless bends are red nodes with two horizontal or vertical edges
std::tuple<Graph, Attributes, Shape>
remove_useless_bends(const Graph& graph, const Attributes& attributes, const Shape& shape) {
    std::vector<bool> kept_nodes(graph.get_number_of_nodes(), true);
    for (size_t node_id : graph.get_nodes_ids()) {
        if (attributes.get_node_color(node_id) == Color::BLACK)
            continue;
        DOMUS_ASSERT(
            graph.get_degree_of_node(node_id) == 2,
            "remove_useless_bends: internal error 1 happened"
        );
        std::array<size_t, 2> edge_ids{graph.get_number_of_nodes(), graph.get_number_of_nodes()};
        size_t i = 0;
        for (const EdgeIter edge : graph.get_edges(node_id)) {
            edge_ids[i++] = edge.id;
        }
        // if the added corner is not flat, keep it
        if (shape.are_parallel(edge_ids[0], edge_ids[1]))
            kept_nodes[node_id] = false;
    }
    Graph new_graph;
    utilities::NodesLabels<size_t> old_id_to_new_id(graph);
    for (size_t node_id : graph.get_nodes_ids()) {
        if (kept_nodes[node_id]) {
            size_t new_id = new_graph.add_node();
            old_id_to_new_id.add_label(node_id, new_id);
        }
    }
    utilities::NodesLabels<size_t> new_id_to_old_id(new_graph);
    for (size_t node_id : graph.get_nodes_ids()) {
        if (kept_nodes[node_id]) {
            size_t new_id = old_id_to_new_id.get_label(node_id);
            new_id_to_old_id.add_label(new_id, node_id);
        }
    }
    Shape new_shape;
    Attributes new_attributes;
    new_attributes.add_attribute(Attribute::NODES_COLOR);
    for (const size_t new_node_id : new_graph.get_nodes_ids()) {
        size_t old_node_id = new_id_to_old_id.get_label(new_node_id);
        new_attributes.set_node_color(new_node_id, attributes.get_node_color(old_node_id));
        for (const EdgeIter edge : graph.get_edges(old_node_id)) {
            size_t old_prev = old_node_id;
            size_t old_curr = edge.neighbor_id;
            // Traverse down the chain of useless nodes until we hit a kept node.
            // Because useless nodes are guaranteed to have degree == 2,
            // there is only one valid "next" node to step to.
            while (!kept_nodes[old_curr]) {
                size_t next_node = get_other_neighbor_id(graph, old_curr, old_prev);
                old_prev = old_curr;
                old_curr = next_node;
            }
            size_t new_curr = old_id_to_new_id.get_label(old_curr);
            if (new_node_id < new_curr) { // check needed to not add the same edge twice
                Direction direction =
                    shape.get_direction(graph, edge.id, old_node_id, edge.neighbor_id);
                size_t new_edge_id = new_graph.add_edge(new_node_id, new_curr);
                new_shape.set_direction(new_edge_id, direction);
            }
        }
    }
    DOMUS_ASSERT(
        is_shape_valid(new_graph, new_shape),
        "remove_useless_bends: built shape is not valid"
    );
    return {std::move(new_graph), std::move(new_attributes), std::move(new_shape)};
}

ShapeMetricsDrawing make_orthogonal_drawing_incremental(Graph& graph, std::vector<Cycle>& cycles);

ShapeMetricsDrawing make_orthogonal_drawing(const Graph& graph) {
    Graph augmented_graph;
    for (size_t i = 0; i < graph.get_number_of_nodes(); ++i)
        augmented_graph.add_node();
    for (size_t node_id : graph.get_nodes_ids())
        for (size_t neighbor_id : graph.get_out_neighbors(node_id))
            augmented_graph.add_edge(node_id, neighbor_id);

    auto cycles = algorithms::compute_cycle_basis(augmented_graph);
    return make_orthogonal_drawing_incremental(augmented_graph, cycles);
}

std::optional<Cycle> check_if_metrics_exist(Shape& shape, Graph& graph) {
    const auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    Ordering ordering = Ordering::build(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x =
        algorithms::find_a_directed_cycle_in_graph(ordering.get_ordering_x());
    std::optional<Cycle> cycle_y =
        algorithms::find_a_directed_cycle_in_graph(ordering.get_ordering_y());
    if (cycle_x.has_value()) {
        DOMUS_ASSERT(
            algorithms::is_cycle_in_graph(ordering.get_ordering_x(), *cycle_x),
            "check_if_metrics_exist: cycle_x is not in ordering_x"
        );
        return build_cycle_in_graph_from_cycle_in_ordering(
            classes_x,
            graph,
            shape,
            cycle_x.value(),
            ordering.get_ordering_x_edge_to_graph_edge(),
            false
        );
    }
    if (cycle_y.has_value()) {
        DOMUS_ASSERT(
            algorithms::is_cycle_in_graph(ordering.get_ordering_y(), *cycle_y),
            "check_if_metrics_exist: cycle_y is not in ordering_y"
        );
        return build_cycle_in_graph_from_cycle_in_ordering(
            classes_y,
            graph,
            shape,
            cycle_y.value(),
            ordering.get_ordering_y_edge_to_graph_edge(),
            true
        );
    }
    return std::nullopt;
}

void build_nodes_positions(Graph& graph, Attributes& attributes, Shape& shape);

bool has_graph_degree_more_than_4(const Graph& graph) {
    for (const size_t node_id : graph.get_nodes_ids())
        if (graph.get_degree_of_node(node_id) > 4)
            return true;
    return false;
}

void add_green_blue_nodes(Graph& graph, Attributes& attributes, Shape& shape);

void make_shifts_overlapped_edges(Graph& graph, Attributes& attributes, Shape& shape);

void fix_negative_positions(const Graph& graph, Attributes& attributes);

void build_nodes_position_degree_more_than_4(
    Graph& augmented_graph, Attributes& attributes, Shape& shape
) {
    add_green_blue_nodes(augmented_graph, attributes, shape);
    build_nodes_positions(augmented_graph, attributes, shape);
    make_shifts_overlapped_edges(augmented_graph, attributes, shape);
    fix_negative_positions(augmented_graph, attributes);
}

ShapeMetricsDrawing make_orthogonal_drawing_incremental(Graph& graph, std::vector<Cycle>& cycles) {
    Attributes attributes;
    attributes.add_attribute(Attribute::NODES_COLOR);
    for (const size_t node_id : graph.get_nodes_ids()) {
        attributes.set_node_color(node_id, Color::BLACK);
    }
    Shape shape = build_shape(graph, attributes, cycles);
    std::optional<Cycle> cycle_to_add = check_if_metrics_exist(shape, graph);
    size_t number_of_added_cycles = 0;
    while (cycle_to_add.has_value()) {
        cycles.push_back(std::move(*cycle_to_add));
        number_of_added_cycles++;
        shape = build_shape(graph, attributes, cycles);
        cycle_to_add = check_if_metrics_exist(shape, graph);
    }
    const size_t old_size = graph.get_number_of_nodes();
    auto [new_graph, new_attributes, new_shape] = remove_useless_bends(graph, attributes, shape);
    graph = std::move(new_graph);
    shape = std::move(new_shape);
    attributes = std::move(new_attributes);
    DOMUS_ASSERT(
        is_shape_valid(graph, shape),
        "make_orthogonal_drawing_incremental: shape is not valid"
    );
    // from now on cycles are not valid anymore (because of removal of useless bends)
    const size_t number_of_cycles = cycles.size();
    cycles.clear();
    const size_t number_of_useless_bends = old_size - graph.get_number_of_nodes();
    if (has_graph_degree_more_than_4(graph))
        build_nodes_position_degree_more_than_4(graph, attributes, shape);
    else
        build_nodes_positions(graph, attributes, shape);
    compact_area(graph, attributes);
    OrthogonalDrawing drawing{std::move(graph), std::move(attributes), std::move(shape)};
    return {
        std::move(drawing),
        number_of_cycles - number_of_added_cycles,
        number_of_added_cycles,
        number_of_useless_bends
    };
}

void find_inconsistencies(Graph& graph, Shape& shape, Attributes& attributes);

void build_nodes_positions(Graph& graph, Attributes& attributes, Shape& shape) {
    find_inconsistencies(graph, shape, attributes);
    auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    Ordering ordering = Ordering::build(classes_x, classes_y, graph, shape);

    auto new_classes_x_ordering =
        algorithms::make_topological_ordering(ordering.get_ordering_x()).value();
    auto new_classes_y_ordering =
        algorithms::make_topological_ordering(ordering.get_ordering_y()).value();
    size_t current_position_x = 0;
    utilities::NodesLabels<size_t> node_id_to_position_x(graph);
    for (size_t class_id : new_classes_x_ordering) {
        for (const size_t node_id : classes_x.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::BLUE)
                current_position_x += 100;
        for (const size_t node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x.add_label(node_id, current_position_x);
        current_position_x += 100;
    }
    size_t current_position_y = 0;
    utilities::NodesLabels<size_t> node_id_to_position_y(graph);
    for (size_t class_id : new_classes_y_ordering) {
        for (const size_t node_id : classes_y.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::GREEN)
                current_position_y += 100;
        for (const size_t node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y.add_label(node_id, current_position_y);
        current_position_y += 100;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (const size_t node_id : graph.get_nodes_ids()) {
        const size_t x = node_id_to_position_x.get_label(node_id);
        const size_t y = node_id_to_position_y.get_label(node_id);
        attributes.set_position(node_id, static_cast<int>(x), static_cast<int>(y));
    }
}

const std::vector<std::optional<std::pair<Edge, Direction>>>
find_edges_to_fix(const Graph& graph, const Shape& shape, const Attributes& attributes) {
    std::vector<std::optional<std::pair<Edge, Direction>>> edge_direction(
        graph.get_number_of_edges(),
        std::nullopt
    );
    for (size_t node_id : graph.get_nodes_ids()) {
        if (graph.get_degree_of_node(node_id) <= 4)
            continue;
        std::optional<size_t> downest_left, downest_right, leftest_up, leftest_down;
        std::optional<size_t> downest_edge_id_left, downest_edge_id_right, leftest_edge_id_up,
            leftest_edge_id_down;
        for (const EdgeIter edge : graph.get_edges(node_id)) {
            const size_t added_id = edge.neighbor_id;
            if (shape.is_horizontal(edge.id)) {
                DOMUS_ASSERT(
                    !shape.is_left(graph, edge.id, node_id, added_id),
                    "find_edges_to_fix: internal errors happened"
                );
                size_t other_neighbor_id = 0;
                size_t other_edge_id = 0;
                for (const EdgeIter e : graph.get_edges(added_id)) {
                    if (e.neighbor_id == node_id)
                        continue;
                    other_neighbor_id = e.neighbor_id;
                    other_edge_id = e.id;
                }
                if (shape.is_up(graph, other_edge_id, added_id, other_neighbor_id)) {
                    if (!leftest_up.has_value()) {
                        leftest_up = added_id;
                        leftest_edge_id_up = edge.id;
                    } else if (attributes.get_position_x(added_id) <
                               attributes.get_position_x(leftest_up.value())) {
                        leftest_up = added_id;
                        leftest_edge_id_up = edge.id;
                    }
                } else {
                    if (!leftest_down.has_value()) {
                        leftest_down = added_id;
                        leftest_edge_id_down = edge.id;
                    } else if (attributes.get_position_x(added_id) <
                               attributes.get_position_x(leftest_down.value())) {
                        leftest_down = added_id;
                        leftest_edge_id_down = edge.id;
                    }
                }
            } else {
                DOMUS_ASSERT(
                    !shape.is_down(graph, edge.id, node_id, added_id),
                    "find_edges_to_fix: internal errors happened"
                );
                size_t other_neighbor_id = 0;
                size_t other_edge_id = 0;
                for (const EdgeIter e : graph.get_edges(added_id)) {
                    if (e.neighbor_id == node_id)
                        continue;
                    other_neighbor_id = e.neighbor_id;
                    other_edge_id = e.id;
                }
                if (shape.is_left(graph, other_edge_id, added_id, other_neighbor_id)) {
                    if (!downest_left.has_value()) {
                        downest_left = added_id;
                        downest_edge_id_left = edge.id;
                    } else if (attributes.get_position_y(added_id) <
                               attributes.get_position_y(downest_left.value())) {
                        downest_left = added_id;
                        downest_edge_id_left = edge.id;
                    }
                } else {
                    if (!downest_right.has_value()) {
                        downest_right = added_id;
                        downest_edge_id_right = edge.id;
                    } else if (attributes.get_position_y(added_id) <
                               attributes.get_position_y(downest_right.value())) {
                        downest_right = added_id;
                        downest_edge_id_right = edge.id;
                    }
                }
            }
        }
        edge_direction[leftest_edge_id_up.value()] = {{node_id, leftest_up.value()}, Direction::UP};
        edge_direction[leftest_edge_id_down.value()] = {
            {node_id, leftest_down.value()},
            Direction::DOWN
        };
        edge_direction[downest_edge_id_left.value()] = {
            {node_id, downest_left.value()},
            Direction::LEFT
        };
        edge_direction[downest_edge_id_right.value()] = {
            {node_id, downest_right.value()},
            Direction::RIGHT
        };
    }
    return edge_direction;
}

// at the moment, a node with degree > 4 doesn't have all its "ports" used,
// this method takes some of its neighbors and places them in the unused "ports"
std::tuple<Graph, Attributes, Shape>
fix_useless_green_blue_nodes(const Graph& graph, const Attributes& attributes, const Shape& shape) {
    const auto edge_to_direction = find_edges_to_fix(graph, shape, attributes);
    utilities::NodesContainer skip_node(graph);
    for (size_t edge_id = 0; edge_id < edge_to_direction.size(); edge_id++) {
        if (!edge_to_direction[edge_id].has_value())
            continue;
        skip_node.add_node(edge_to_direction[edge_id]->first.to_id);
    }

    Graph new_graph;
    utilities::NodesLabels<size_t> old_id_to_new_id(graph);
    Shape new_shape;
    Attributes new_attributes;
    new_attributes.add_attribute(Attribute::NODES_COLOR);

    for (size_t node_id : graph.get_nodes_ids())
        if (!skip_node.has_node(node_id)) {
            size_t new_id = new_graph.add_node();
            old_id_to_new_id.add_label(node_id, new_id);
            new_attributes.set_node_color(new_id, attributes.get_node_color(node_id));
        }

    for (size_t node_id : graph.get_nodes_ids()) {
        if (skip_node.has_node(node_id))
            continue;
        for (auto [edge_id, neighbor_id] : graph.get_edges(node_id)) {
            if (!skip_node.has_node(neighbor_id)) {
                if (node_id > neighbor_id)
                    continue;
                size_t new_node_id = old_id_to_new_id.get_label(node_id);
                size_t new_neighbor_id = old_id_to_new_id.get_label(neighbor_id);
                size_t new_edge_id = new_graph.add_edge(new_node_id, new_neighbor_id);
                Direction direction = shape.get_direction(graph, edge_id, node_id, neighbor_id);
                new_shape.set_direction(new_edge_id, direction);
                continue;
            }
            DOMUS_ASSERT(
                attributes.get_node_color(neighbor_id) == Color::GREEN ||
                    attributes.get_node_color(neighbor_id) == Color::BLUE,
                "fix_useless_green_blue_nodes: internal error - node to remove which is "
                "neither green nor blue (color = {})",
                color_to_string(attributes.get_node_color(neighbor_id))
            );
            if (graph.get_degree_of_node(node_id) <= 4)
                continue;
            Direction direction = edge_to_direction.at(edge_id)->second;
            size_t curr = node_id;
            size_t next = neighbor_id;
            size_t other = get_other_neighbor_id(graph, next, curr);
            while (skip_node.has_node(other)) {
                curr = next;
                next = other;
                other = get_other_neighbor_id(graph, next, curr);
            }
            size_t new_node_id = old_id_to_new_id.get_label(node_id);
            size_t new_neighbor_id = old_id_to_new_id.get_label(other);
            if (!new_graph.are_neighbors(new_node_id, new_neighbor_id)) {
                size_t new_edge_id = new_graph.add_edge(new_node_id, new_neighbor_id);
                new_shape.set_direction(new_edge_id, direction);
            }
        }
    }
    DOMUS_ASSERT(
        graph.get_number_of_edges() - graph.get_number_of_nodes() ==
            new_graph.get_number_of_edges() - new_graph.get_number_of_nodes(),
        "fix_useless_green_blue_nodes: internal error - new graph does not have matching size "
        "with "
        "old graph"
    );
    return std::make_tuple(std::move(new_graph), std::move(new_attributes), std::move(new_shape));
}

void add_green_blue_nodes(Graph& graph, Attributes& attributes, Shape& shape) {
    const auto nodes = graph.get_nodes_ids() | std::views::filter([&graph](size_t node_id) {
                           return graph.get_degree_of_node(node_id) > 4;
                       }) |
                       std::ranges::to<std::vector<size_t>>();

    for (const size_t node_id : nodes) {
        const std::vector<EdgeIter> edges_to_subdivide =
            std::ranges::to<std::vector<EdgeIter>>(graph.get_edges(node_id));
        for (const EdgeIter edge : edges_to_subdivide) {
            const Direction direction =
                shape.get_direction(graph, edge.id, node_id, edge.neighbor_id);
            shape.remove_direction(edge.id);
            const Subdivision s = graph.subdivide_edge(edge.id);
            const size_t edge_id_1 =
                (s.from_id == node_id) ? s.edge_from_between_id : s.edge_between_to_id;
            const size_t edge_id_2 =
                (s.from_id == node_id) ? s.edge_between_to_id : s.edge_from_between_id;

            shape.set_direction(graph, edge_id_2, s.in_between_id, edge.neighbor_id, direction);
            if (shape::is_horizontal(direction)) {
                attributes.set_node_color(s.in_between_id, Color::GREEN);
                shape.set_direction(graph, edge_id_1, node_id, s.in_between_id, Direction::UP);
            } else {
                attributes.set_node_color(s.in_between_id, Color::BLUE);
                shape.set_direction(graph, edge_id_1, node_id, s.in_between_id, Direction::RIGHT);
            }
        }
    }
    const auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    const Ordering ordering = Ordering::build(classes_x, classes_y, graph, shape);
    const Graph& ordering_x = ordering.get_ordering_x();
    const Graph& ordering_y = ordering.get_ordering_y();
    const std::vector<size_t> classes_x_ordering =
        algorithms::make_topological_ordering(ordering_x).value();
    const std::vector<size_t> classes_y_ordering =
        algorithms::make_topological_ordering(ordering_y).value();
    size_t current_position_x = 0;
    utilities::NodesLabels<size_t> node_id_to_position_x(graph);
    for (const size_t class_id : classes_x_ordering) {
        for (const size_t node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x.add_label(node_id, 100 * current_position_x);
        ++current_position_x;
    }
    size_t current_position_y = 0;
    utilities::NodesLabels<size_t> node_id_to_position_y(graph);
    for (const size_t class_id : classes_y_ordering) {
        for (const size_t node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y.add_label(node_id, 100 * current_position_y);
        ++current_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (const size_t node_id : graph.get_nodes_ids()) {
        const size_t x = node_id_to_position_x.get_label(node_id);
        const size_t y = node_id_to_position_y.get_label(node_id);
        attributes.set_position(node_id, static_cast<int>(x), static_cast<int>(y));
    }
    const auto [new_graph, new_attributes, new_shape] =
        fix_useless_green_blue_nodes(graph, attributes, shape);
    graph = std::move(new_graph);
    attributes = std::move(new_attributes);
    shape = std::move(new_shape);
}

void fix_inconsistency(
    const Cycle& cycle,
    Attributes& attributes,
    const Graph& graph,
    Shape& shape,
    const Color color_to_find
) {
    const Direction direction = color_to_find == Color::GREEN ? Direction::UP : Direction::RIGHT;
    const Color dark_color = color_to_find == Color::GREEN ? Color::GREEN_DARK : Color::BLUE_DARK;
    std::optional<size_t> colored_node;
    for (const size_t node_id : cycle.get_nodes_ids()) {
        if (attributes.get_node_color(node_id) != color_to_find)
            continue;
        colored_node = node_id;
        break;
    }
    DOMUS_ASSERT(colored_node.has_value(), "fix_inconsistency: internal error happened");
    size_t colored_node_id = colored_node.value();
    size_t neighbors_ids[2] = {graph.get_number_of_nodes(), graph.get_number_of_nodes()};
    size_t edge_ids[2];
    int i = 0;
    for (const EdgeIter edge : graph.get_edges(colored_node_id)) {
        neighbors_ids[i] = edge.neighbor_id;
        edge_ids[i] = edge.id;
        ++i;
    }
    if (shape.is_up(graph, edge_ids[0], neighbors_ids[0], colored_node_id)) {
        shape.remove_direction(edge_ids[0]);
        shape.set_direction(graph, edge_ids[0], colored_node_id, neighbors_ids[0], direction);
    } else {
        shape.remove_direction(edge_ids[1]);
        shape.set_direction(graph, edge_ids[1], colored_node_id, neighbors_ids[1], direction);
    }
    attributes.change_node_color(colored_node_id, dark_color);
}

void find_inconsistencies(Graph& graph, Shape& shape, Attributes& attributes) {
    auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    Ordering ordering = Ordering::build(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x =
        algorithms::find_a_directed_cycle_in_graph(ordering.get_ordering_x());
    std::optional<Cycle> cycle_y =
        algorithms::find_a_directed_cycle_in_graph(ordering.get_ordering_y());
    if (cycle_x.has_value() || cycle_y.has_value()) {
        if (cycle_x.has_value()) {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(
                classes_x,
                graph,
                shape,
                cycle_x.value(),
                ordering.get_ordering_x_edge_to_graph_edge(),
                false
            );
            fix_inconsistency(cycle, attributes, graph, shape, Color::BLUE);
        } else {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(
                classes_x,
                graph,
                shape,
                cycle_y.value(),
                ordering.get_ordering_y_edge_to_graph_edge(),
                true
            );
            fix_inconsistency(cycle, attributes, graph, shape, Color::GREEN);
        }
        find_inconsistencies(graph, shape, attributes);
    }
}

template <typename Func>
void shifting_order(
    size_t node_id,
    Graph& graph,
    Shape& shape,
    std::vector<size_t>& nodes_at_direction,
    Attributes& attributes,
    const Direction increasing_direction,
    Func get_position
) {
    const Direction decreasing_direction = opposite_direction(increasing_direction);
    std::sort(nodes_at_direction.begin(), nodes_at_direction.end(), [&](size_t a, size_t b) {
        if (attributes.get_node_color(a) == Color::BLACK) {
            auto [b_other_neighbor_id, b_other_edge] = get_other_edge_id(graph, b, node_id);
            return shape.get_direction(graph, b_other_edge, b, b_other_neighbor_id) ==
                   increasing_direction;
        }
        if (attributes.get_node_color(b) == Color::BLACK) {
            auto [a_other_neighbor_id, a_other_edge] = get_other_edge_id(graph, a, node_id);
            return shape.get_direction(graph, a_other_edge, a, a_other_neighbor_id) ==
                   decreasing_direction;
        }
        auto [a_other_neighbor, a_other_edge] = get_other_edge_id(graph, a, node_id);
        auto [b_other_neighbor, b_other_edge] = get_other_edge_id(graph, b, node_id);
        if (shape.get_direction(graph, a_other_edge, a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(graph, b_other_edge, b, b_other_neighbor) == decreasing_direction) {
            return false;
        }
        if (shape.get_direction(graph, a_other_edge, a, a_other_neighbor) == decreasing_direction &&
            shape.get_direction(graph, b_other_edge, b, b_other_neighbor) == increasing_direction) {
            return true;
        }
        if (shape.get_direction(graph, a_other_edge, a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(graph, b_other_edge, b, b_other_neighbor) == increasing_direction) {
            return get_position(attributes, a) > get_position(attributes, b);
        }
        return get_position(attributes, a) < get_position(attributes, b);
    });
}

size_t
find_fixed_index_node(const Attributes& attributes, const std::vector<size_t>& nodes_at_direction) {
    for (size_t i = 0; i < nodes_at_direction.size(); ++i) {
        size_t node_id = nodes_at_direction[i];
        if (attributes.get_node_color(node_id) == Color::BLACK)
            return i;
    }
    return nodes_at_direction.size() / 2;
}

enum class Axis { X, Y };

void make_shifts(
    size_t node_id,
    Graph& graph,
    Shape& shape,
    Attributes& attributes,
    std::vector<size_t>& nodes_at_direction,
    const Axis axis,
    const Direction increasing_direction,
    const Color color
) {
    auto position_function =
        axis == Axis::X ? [](const Attributes& a, size_t id) { return a.get_position_x(id); }
                        : [](const Attributes& a, size_t id) { return a.get_position_y(id); };
    shifting_order(
        node_id,
        graph,
        shape,
        nodes_at_direction,
        attributes,
        increasing_direction,
        position_function
    );
    const auto position_function_other =
        axis == Axis::X ? [](const Attributes& a, size_t id) { return a.get_position_y(id); }
                        : [](const Attributes& a, size_t id) { return a.get_position_x(id); };
    const auto change_position_other =
        axis == Axis::X
            ? [](Attributes& a, size_t id, int value) { a.change_position_y(id, value); }
            : [](Attributes& a, size_t id, int value) { a.change_position_x(id, value); };
    size_t index_of_fixed_node = find_fixed_index_node(attributes, nodes_at_direction);
    int initial_position = position_function_other(attributes, node_id);
    for (const size_t id : graph.get_nodes_ids()) {
        int old_position_y = position_function_other(attributes, id);
        if (old_position_y > initial_position) {
            int node_count = static_cast<int>(nodes_at_direction.size());
            int offset = node_count - static_cast<int>(index_of_fixed_node) - 1;
            int new_position_y = old_position_y + 5 * offset;
            change_position_other(attributes, id, new_position_y);
        }
        if (old_position_y < initial_position) {
            const int new_position_y = old_position_y - 5 * static_cast<int>(index_of_fixed_node);
            change_position_other(attributes, id, new_position_y);
        }
    }
    for (size_t i = 0; i < nodes_at_direction.size(); ++i) {
        if (i == index_of_fixed_node)
            continue;
        size_t node_to_shift_id = nodes_at_direction[i];
        int shift = (static_cast<int>(i) - static_cast<int>(index_of_fixed_node)) * 5;
        auto [node_to_shift_neighbor_id, node_to_shift_edge_id] =
            get_other_edge_id(graph, node_to_shift_id, node_id);
        Direction direction = shape.get_direction(
            graph,
            node_to_shift_edge_id,
            node_to_shift_id,
            node_to_shift_neighbor_id
        );
        size_t added_node_id = graph.add_node();
        attributes.set_node_color(added_node_id, color);
        size_t edge_id_0 = graph.add_edge(node_id, added_node_id);
        size_t edge_id_1 = graph.add_edge(added_node_id, node_to_shift_id);
        shape.set_direction(edge_id_0, direction);
        shape.set_direction(edge_id_1, direction);
        DOMUS_ASSERT(
            graph.are_neighbors(node_id, node_to_shift_id),
            "make_shifts: internal error happened"
        );
        size_t edge_to_remove_id;
        if (graph.has_edge(node_id, node_to_shift_id))
            edge_to_remove_id = graph.remove_edge(node_id, node_to_shift_id);
        else
            edge_to_remove_id = graph.remove_edge(node_to_shift_id, node_id);
        shape.remove_direction(edge_to_remove_id);
        if (axis == Axis::X)
            attributes.set_position(
                added_node_id,
                attributes.get_position_x(node_id),
                initial_position + shift
            );
        else
            attributes.set_position(
                added_node_id,
                initial_position + shift,
                attributes.get_position_y(node_id)
            );
        change_position_other(
            attributes,
            node_to_shift_id,
            position_function_other(attributes, added_node_id)
        );
    }
}

auto neighbors_at_each_direction(const Graph& graph, size_t node_id, const Shape& shape) {
    std::array<std::vector<size_t>, 4> nodes_at_direction{};
    for (const EdgeIter edge : graph.get_edges(node_id)) {
        Direction dir = shape.get_direction(graph, edge.id, node_id, edge.neighbor_id);
        switch (dir) {
        case Direction::RIGHT:
            nodes_at_direction[0].push_back(edge.neighbor_id);
            break;
        case Direction::UP:
            nodes_at_direction[1].push_back(edge.neighbor_id);
            break;
        case Direction::LEFT:
            nodes_at_direction[2].push_back(edge.neighbor_id);
            break;
        case Direction::DOWN:
            nodes_at_direction[3].push_back(edge.neighbor_id);
            break;
        default:
            DOMUS_ASSERT(false, "neighbors_at_each_direction: found invalid direction");
            break;
        }
    }
    return nodes_at_direction;
}

void make_shifts_overlapped_edges(Graph& graph, Attributes& attributes, Shape& shape) {
    std::vector<size_t> nodes;
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    }
    for (size_t node_id : nodes) {
        auto nodes_to_sort = neighbors_at_each_direction(graph, node_id, shape);
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[0],
            Axis::X,
            Direction::UP,
            Color::GREEN
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[1],
            Axis::Y,
            Direction::RIGHT,
            Color::BLUE
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[2],
            Axis::X,
            Direction::UP,
            Color::GREEN_DARK
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[3],
            Axis::Y,
            Direction::RIGHT,
            Color::BLUE_DARK
        );
    }
}

void fix_negative_positions(const Graph& graph, Attributes& attributes) {
    if (graph.get_number_of_nodes() == 0)
        return;
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    for (const size_t node_id : graph.get_nodes_ids()) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    }
    if (min_x < 0)
        for (const size_t node_id : graph.get_nodes_ids()) {
            attributes.change_position_x(node_id, attributes.get_position_x(node_id) - min_x);
        }
    if (min_y < 0)
        for (const size_t node_id : graph.get_nodes_ids()) {
            attributes.change_position_y(node_id, attributes.get_position_y(node_id) - min_y);
        }
}

} // namespace domus::orthogonal