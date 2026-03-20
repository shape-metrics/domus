#include "domus/orthogonal/drawing_builder.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <limits.h>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "domus/core/color.hpp"
#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/orthogonal/area_compacter.hpp"
#include "domus/orthogonal/shape/direction.hpp"
#include "domus/orthogonal/shape/shape.hpp"
#include "domus/orthogonal/shape/shape_builder.hpp"

#include "../core/domus_debug.hpp"
#include "../nlohmann/json.hpp"
#include "equivalence_classes.hpp"

std::vector<size_t>
path_in_class(const Graph& graph, size_t from, size_t to, const Shape& shape, bool go_horizontal) {
    std::vector<size_t> path;
    NodesContainer visited(graph);
    std::function<void(size_t)> dfs = [&](size_t current) {
        if (current == to) {
            path.push_back(current);
            return;
        }
        bool stop = false;
        visited.add_node(current);
        graph.for_each_edge(current, [&](size_t edge_id, size_t neighbor_id) {
            if (stop)
                return;
            if (visited.has_node(neighbor_id))
                return;
            if (go_horizontal == shape.is_horizontal(edge_id)) {
                dfs(neighbor_id);
                if (!path.empty()) {
                    path.push_back(current);
                    stop = true;
                }
            }
        });
        visited.erase(current);
    };
    dfs(from);
    std::ranges::reverse(path);
    return path;
}

std::pair<size_t, size_t>
get_other_edge_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    DOMUS_ASSERT(
        graph.get_degree_of_node(node_id) == 2,
        "get_other_neighbor_id: function only for degree 2 nodes"
    );
    std::optional<size_t> other;
    std::optional<size_t> other_edge_id;
    graph.for_each_edge(node_id, [&](size_t edge_id, size_t other_id) {
        if (other.has_value())
            return;
        if (other_id != neighbor_id) {
            other = other_id;
            other_edge_id = edge_id;
        }
    });
    DOMUS_ASSERT(
        other.has_value(),
        "get_other_neighbor_id: internal error happened, no other neighbor found for node"
    );
    return {other.value(), other_edge_id.value()};
}

size_t get_other_neighbor_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    return get_other_edge_id(graph, node_id, neighbor_id).first;
}

Cycle build_cycle_in_graph_from_cycle_in_ordering(
    const Graph& graph,
    const Shape& shape,
    const Cycle& cycle_in_ordering,
    const std::unordered_map<Edge, Edge, edge_hash>& ordering_edge_to_graph_edge,
    bool go_horizontal
) {
    std::vector<size_t> cycle;
    for (size_t i = 0; i < cycle_in_ordering.size(); ++i) {
        size_t class_id = cycle_in_ordering[i];
        size_t next_class_id = cycle_in_ordering[i + 1];
        auto [from, to] = ordering_edge_to_graph_edge.at({class_id, next_class_id});
        cycle.push_back(from);
        size_t next_next_class_id = cycle_in_ordering[i + 2];
        auto [next_from, next_to] =
            ordering_edge_to_graph_edge.at({next_class_id, next_next_class_id});
        if (to != next_from) {
            std::vector<size_t> path = path_in_class(graph, to, next_from, shape, go_horizontal);
            const size_t end = static_cast<size_t>(static_cast<int>(path.size()) - 1);
            for (size_t j = 0; j < end; ++j)
                cycle.push_back(path[j]);
        }
    }
    return Cycle(cycle);
}

// useless bends are red nodes with two horizontal or vertical edges
std::tuple<Graph, GraphAttributes, Shape>
remove_useless_bends(const Graph& graph, const GraphAttributes& attributes, const Shape& shape) {
    std::vector<bool> kept_nodes(graph.get_number_of_nodes(), true);
    graph.for_each_node([&](size_t node_id) {
        if (attributes.get_node_color(node_id) == Color::BLACK)
            return;
        DOMUS_ASSERT(
            graph.get_degree_of_node(node_id) == 2,
            "remove_useless_bends: internal error 1 happened"
        );
        std::array<size_t, 2> edge_ids{graph.get_number_of_nodes(), graph.get_number_of_nodes()};
        size_t i = 0;
        graph.for_each_edge(node_id, [&edge_ids, &i](size_t edge_id, size_t) {
            edge_ids[i++] = edge_id;
        });
        // if the added corner is not flat, keep it
        if (shape.are_parallel(edge_ids[0], edge_ids[1]))
            kept_nodes[node_id] = false;
    });
    Graph new_graph;
    NodesLabels old_id_to_new_id(graph);
    graph.for_each_node([&](size_t node_id) {
        if (kept_nodes[node_id]) {
            size_t new_id = new_graph.add_node();
            old_id_to_new_id.add_label(node_id, new_id);
        }
    });
    NodesLabels new_id_to_old_id(new_graph);
    graph.for_each_node([&](size_t node_id) {
        if (kept_nodes[node_id]) {
            size_t new_id = old_id_to_new_id.get_label(node_id);
            new_id_to_old_id.add_label(new_id, node_id);
        }
    });
    Shape new_shape;
    GraphAttributes new_attributes;
    new_attributes.add_attribute(Attribute::NODES_COLOR);
    new_graph.for_each_node([&](size_t new_node_id) {
        size_t old_node_id = new_id_to_old_id.get_label(new_node_id);
        new_attributes.set_node_color(new_node_id, attributes.get_node_color(old_node_id));
        graph.for_each_edge(old_node_id, [&](size_t edge_id, size_t old_neighbor_id) {
            size_t old_prev = old_node_id;
            size_t old_curr = old_neighbor_id;
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
                    shape.get_direction(graph, edge_id, old_node_id, old_neighbor_id);
                size_t new_edge_id = new_graph.add_edge(new_node_id, new_curr);
                new_shape.set_direction(new_edge_id, direction);
            }
        });
    });
    DOMUS_ASSERT(
        is_shape_valid(new_graph, new_shape),
        "remove_useless_bends: built shape is not valid"
    );
    return {std::move(new_graph), std::move(new_attributes), std::move(new_shape)};
}

ShapeMetricsDrawing
make_orthogonal_drawing_incremental(const Graph& graph, std::vector<Cycle>& cycles);

std::expected<ShapeMetricsDrawing, std::string> make_orthogonal_drawing(const Graph& graph) {
    auto cycles = compute_cycle_basis(graph);
    return make_orthogonal_drawing_incremental(graph, cycles);
}

std::optional<Cycle> check_if_metrics_exist(Shape& shape, Graph& graph) {
    const auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x = find_a_directed_cycle_in_graph(ordering_x);
    std::optional<Cycle> cycle_y = find_a_directed_cycle_in_graph(ordering_y);
    if (cycle_x.has_value()) {
        return build_cycle_in_graph_from_cycle_in_ordering(
            graph,
            shape,
            cycle_x.value(),
            ordering_x_edge_to_graph_edge,
            false
        );
    }
    if (cycle_y.has_value()) {
        return build_cycle_in_graph_from_cycle_in_ordering(
            graph,
            shape,
            cycle_y.value(),
            ordering_y_edge_to_graph_edge,
            true
        );
    }
    return std::nullopt;
}

void build_nodes_positions(Graph& graph, GraphAttributes& attributes, Shape& shape);

bool has_graph_degree_more_than_4(const Graph& graph) {
    bool has_degree_more_than_4 = false;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) > 4)
            has_degree_more_than_4 = true;
    });
    return has_degree_more_than_4;
}

void add_green_blue_nodes(Graph& graph, GraphAttributes& attributes, Shape& shape);

void make_shifts_overlapped_edges(Graph& graph, GraphAttributes& attributes, Shape& shape);

void fix_negative_positions(const Graph& graph, GraphAttributes& attributes);

void build_nodes_position_degree_more_than_4(
    Graph& augmented_graph, GraphAttributes& attributes, Shape& shape
) {
    add_green_blue_nodes(augmented_graph, attributes, shape);
    build_nodes_positions(augmented_graph, attributes, shape);
    make_shifts_overlapped_edges(augmented_graph, attributes, shape);
    fix_negative_positions(augmented_graph, attributes);
}

ShapeMetricsDrawing
make_orthogonal_drawing_incremental(const Graph& graph, std::vector<Cycle>& cycles) {
    Graph augmented_graph;
    GraphAttributes attributes;
    attributes.add_attribute(Attribute::NODES_COLOR);
    graph.for_each_node([&](size_t node_id) {
        augmented_graph.add_node();
        attributes.set_node_color(node_id, Color::BLACK);
    });
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (node_id < neighbor_id)
                augmented_graph.add_edge(node_id, neighbor_id);
        });
    });
    Shape shape = build_shape(augmented_graph, attributes, cycles);
    std::optional<Cycle> cycle_to_add = check_if_metrics_exist(shape, augmented_graph);
    size_t number_of_added_cycles = 0;
    while (cycle_to_add.has_value()) {
        cycles.push_back(std::move(*cycle_to_add));
        number_of_added_cycles++;
        shape = build_shape(augmented_graph, attributes, cycles);
        cycle_to_add = check_if_metrics_exist(shape, augmented_graph);
    }
    const size_t old_size = augmented_graph.get_number_of_nodes();
    auto [new_graph, new_attributes, new_shape] =
        remove_useless_bends(augmented_graph, attributes, shape);
    augmented_graph = std::move(new_graph);
    shape = std::move(new_shape);
    attributes = std::move(new_attributes);
    DOMUS_ASSERT(is_shape_valid(augmented_graph, shape), "porcoddio");
    // from now on cycles are not valid anymore (because of removal of useless bends)
    const size_t number_of_cycles = cycles.size();
    cycles.clear();
    const size_t number_of_useless_bends = old_size - augmented_graph.get_number_of_nodes();
    if (has_graph_degree_more_than_4(augmented_graph))
        build_nodes_position_degree_more_than_4(augmented_graph, attributes, shape);
    else
        build_nodes_positions(augmented_graph, attributes, shape);
    compact_area(augmented_graph, attributes);
    OrthogonalDrawing drawing{std::move(augmented_graph), std::move(attributes), std::move(shape)};
    return {
        std::move(drawing),
        number_of_cycles - number_of_added_cycles,
        number_of_added_cycles,
        number_of_useless_bends
    };
}

void find_inconsistencies(Graph& graph, Shape& shape, GraphAttributes& attributes);

void build_nodes_positions(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    find_inconsistencies(graph, shape, attributes);
    auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    auto [ordering_x, ordering_y, ignored_1, ignored_2] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    auto new_classes_x_ordering = make_topological_ordering(ordering_x).value();
    auto new_classes_y_ordering = make_topological_ordering(ordering_y).value();
    int current_position_x = -100;
    std::unordered_map<size_t, int> node_id_to_position_x;
    for (size_t class_id : new_classes_x_ordering) {
        int next_position_x = current_position_x + 100;

        classes_x.for_each_elem_of_class(class_id, [&](size_t node_id) {
            if (attributes.get_node_color(node_id) == Color::BLUE)
                next_position_x = current_position_x + 100;
        });
        classes_x.for_each_elem_of_class(class_id, [&](size_t node_id) {
            node_id_to_position_x[node_id] = next_position_x;
        });
        current_position_x = next_position_x;
    }
    int current_position_y = -100;
    std::unordered_map<size_t, int> node_id_to_position_y;
    for (size_t class_id : new_classes_y_ordering) {
        int next_position_y = current_position_y + 100;
        classes_y.for_each_elem_of_class(class_id, [&](size_t node_id) {
            if (attributes.get_node_color(node_id) == Color::GREEN)
                next_position_y = current_position_y + 100;
        });
        classes_y.for_each_elem_of_class(class_id, [&](size_t node_id) {
            node_id_to_position_y[node_id] = next_position_y;
        });
        current_position_y = next_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    graph.for_each_node([&](size_t node_id) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    });
}

const std::vector<std::optional<std::pair<Edge, Direction>>>
find_edges_to_fix(const Graph& graph, const Shape& shape, const GraphAttributes& attributes) {
    std::vector<std::optional<std::pair<Edge, Direction>>> edge_direction(
        graph.get_number_of_edges(),
        std::nullopt
    );
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) <= 4)
            return;
        std::optional<size_t> downest_left, downest_right, leftest_up, leftest_down;
        std::optional<size_t> downest_edge_id_left, downest_edge_id_right, leftest_edge_id_up,
            leftest_edge_id_down;
        graph.for_each_edge(node_id, [&](size_t edge_id, size_t added_id) {
            if (shape.is_horizontal(edge_id)) {
                DOMUS_ASSERT(
                    !shape.is_left(graph, edge_id, node_id, added_id),
                    "find_edges_to_fix: internal errors happened"
                );
                size_t other_neighbor_id = 0;
                size_t other_edge_id = 0;
                graph.for_each_edge(added_id, [&](size_t e_id, size_t neighbor_id) {
                    if (neighbor_id == node_id)
                        return;
                    other_neighbor_id = neighbor_id;
                    other_edge_id = e_id;
                });
                if (shape.is_up(graph, other_edge_id, added_id, other_neighbor_id)) {
                    if (!leftest_up.has_value()) {
                        leftest_up = added_id;
                        leftest_edge_id_up = edge_id;
                    } else if (attributes.get_position_x(added_id) <
                               attributes.get_position_x(leftest_up.value())) {
                        leftest_up = added_id;
                        leftest_edge_id_up = edge_id;
                    }
                } else {
                    if (!leftest_down.has_value()) {
                        leftest_down = added_id;
                        leftest_edge_id_down = edge_id;
                    } else if (attributes.get_position_x(added_id) <
                               attributes.get_position_x(leftest_down.value())) {
                        leftest_down = added_id;
                        leftest_edge_id_down = edge_id;
                    }
                }
            } else {
                DOMUS_ASSERT(
                    !shape.is_down(graph, edge_id, node_id, added_id),
                    "find_edges_to_fix: internal errors happened"
                );
                size_t other_neighbor_id = 0;
                size_t other_edge_id = 0;
                graph.for_each_edge(added_id, [&](size_t e_id, size_t neighbor_id) {
                    if (neighbor_id == node_id)
                        return;
                    other_neighbor_id = neighbor_id;
                    other_edge_id = e_id;
                });
                if (shape.is_left(graph, other_edge_id, added_id, other_neighbor_id)) {
                    if (!downest_left.has_value()) {
                        downest_left = added_id;
                        downest_edge_id_left = edge_id;
                    } else if (attributes.get_position_y(added_id) <
                               attributes.get_position_y(downest_left.value())) {
                        downest_left = added_id;
                        downest_edge_id_left = edge_id;
                    }
                } else {
                    if (!downest_right.has_value()) {
                        downest_right = added_id;
                        downest_edge_id_right = edge_id;
                    } else if (attributes.get_position_y(added_id) <
                               attributes.get_position_y(downest_right.value())) {
                        downest_right = added_id;
                        downest_edge_id_right = edge_id;
                    }
                }
            }
        });
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
    });
    return edge_direction;
}

// at the moment, a node with degree > 4 doesn't have all its "ports" used,
// this method takes some of its neighbors and places them in the unused "ports"
std::tuple<Graph, GraphAttributes, Shape> fix_useless_green_blue_nodes(
    const Graph& graph, const GraphAttributes& attributes, const Shape& shape
) {
    const auto edge_to_direction = find_edges_to_fix(graph, shape, attributes);

    std::vector<bool> keep_node(graph.get_number_of_nodes(), true);
    for (size_t edge_id = 0; edge_id < edge_to_direction.size(); edge_id++) {
        if (!edge_to_direction[edge_id].has_value())
            continue;
        keep_node[edge_to_direction[edge_id]->first.to_id] = false;
    }

    Graph new_graph;
    NodesLabels old_id_to_new_id(graph);
    Shape new_shape;
    GraphAttributes new_attributes;
    new_attributes.add_attribute(Attribute::NODES_COLOR);

    graph.for_each_node([&](size_t node_id) {
        if (keep_node[node_id]) {
            size_t new_id = new_graph.add_node();
            old_id_to_new_id.add_label(node_id, new_id);
            new_attributes.set_node_color(new_id, attributes.get_node_color(node_id));
        }
    });

    graph.for_each_node([&](size_t node_id) {
        if (!keep_node[node_id])
            return;
        graph.for_each_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
            if (!keep_node[neighbor_id]) {
                DOMUS_ASSERT(
                    attributes.get_node_color(neighbor_id) == Color::GREEN ||
                        attributes.get_node_color(neighbor_id) == Color::BLUE,
                    "fix_useless_green_blue_nodes: internal error - node to remove which is "
                    "neither green nor blue (color = {})",
                    color_to_string(attributes.get_node_color(neighbor_id))
                );
                if (graph.get_degree_of_node(node_id) <= 4)
                    return;
                Direction direction = edge_to_direction.at(edge_id)->second;
                size_t curr = node_id;
                size_t next = neighbor_id;
                size_t other = get_other_neighbor_id(graph, next, curr);
                while (!keep_node[other]) {
                    curr = next;
                    next = other;
                    other = get_other_neighbor_id(graph, next, curr);
                }
                if (!new_graph.are_neighbors(node_id, other)) {
                    size_t new_node_id = old_id_to_new_id.get_label(node_id);
                    size_t new_neighbor_id = old_id_to_new_id.get_label(other);
                    size_t new_edge_id = new_graph.add_edge(new_node_id, new_neighbor_id);
                    new_shape.set_direction(new_edge_id, direction);
                }
                return;
            }
            if (node_id < neighbor_id) {
                size_t new_node_id = old_id_to_new_id.get_label(node_id);
                size_t new_neighbor_id = old_id_to_new_id.get_label(neighbor_id);
                size_t new_edge_id = new_graph.add_edge(new_node_id, new_neighbor_id);
                Direction direction = shape.get_direction(graph, edge_id, node_id, neighbor_id);
                new_shape.set_direction(new_edge_id, direction);
            }
        });
    });
    DOMUS_ASSERT(
        graph.get_number_of_edges() - graph.get_number_of_nodes() ==
            new_graph.get_number_of_edges() - new_graph.get_number_of_nodes(),
        "fix_useless_green_blue_nodes: internal error - new graph does not have matching size with "
        "old graph"
    );
    return std::make_tuple(std::move(new_graph), std::move(new_attributes), std::move(new_shape));
}

void add_green_blue_nodes(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    std::vector<size_t> nodes;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    });
    for (size_t node_id : nodes) {
        std::vector<size_t> edge_ids_to_remove;
        std::vector<std::pair<Edge, Direction>> edges_to_add;
        graph.for_each_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
            size_t added_id = graph.add_node();
            Direction direction = shape.get_direction(graph, edge_id, node_id, neighbor_id);
            edges_to_add.push_back({{added_id, neighbor_id}, direction});
            if (shape.is_horizontal(edge_id)) {
                attributes.set_node_color(added_id, Color::GREEN);
                edges_to_add.push_back({{node_id, added_id}, Direction::UP});
            } else {
                attributes.set_node_color(added_id, Color::BLUE);
                edges_to_add.push_back({{node_id, added_id}, Direction::RIGHT});
            }
            edge_ids_to_remove.emplace_back(edge_id);
        });
        for (size_t edge_id : edge_ids_to_remove) {
            shape.remove_direction(edge_id);
            graph.remove_edge(edge_id);
        }
        for (auto [edge, direction] : edges_to_add) {
            size_t edge_id = graph.add_edge(edge.from_id, edge.to_id);
            shape.set_direction(edge_id, direction);
        }
    }
    auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    auto ordering = equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    Graph& ordering_x = std::get<0>(ordering);
    Graph& ordering_y = std::get<1>(ordering);
    std::vector<size_t> classes_x_ordering = make_topological_ordering(ordering_x).value();
    std::vector<size_t> classes_y_ordering = make_topological_ordering(ordering_y).value();
    int current_position_x = 0;
    std::unordered_map<size_t, int> node_id_to_position_x;
    for (size_t class_id : classes_x_ordering) {
        classes_x.for_each_elem_of_class(class_id, [&](size_t node_id) {
            node_id_to_position_x[node_id] = 100 * current_position_x;
        });
        ++current_position_x;
    }
    int current_position_y = 0;
    std::unordered_map<size_t, int> node_id_to_position_y;
    for (size_t class_id : classes_y_ordering) {
        classes_y.for_each_elem_of_class(class_id, [&](size_t node_id) {
            node_id_to_position_y[node_id] = 100 * current_position_y;
        });
        ++current_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    graph.for_each_node([&](size_t node_id) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    });
    auto [new_graph, new_attributes, new_shape] =
        fix_useless_green_blue_nodes(graph, attributes, shape);
    graph = std::move(new_graph);
    attributes = std::move(new_attributes);
    shape = std::move(new_shape);
}

void fix_inconsistency(
    const Cycle& cycle,
    GraphAttributes& attributes,
    const Graph& graph,
    Shape& shape,
    Color color_to_find
) {
    const Direction direction = color_to_find == Color::GREEN ? Direction::UP : Direction::RIGHT;
    const Color dark_color = color_to_find == Color::GREEN ? Color::GREEN_DARK : Color::BLUE_DARK;
    std::optional<size_t> colored_node;
    cycle.for_each([&](size_t node_id) {
        if (attributes.get_node_color(node_id) != color_to_find)
            return;
        colored_node = node_id;
    });
    DOMUS_ASSERT(colored_node.has_value(), "fix_inconsistency: internal error happened");
    size_t colored_node_id = colored_node.value();
    size_t neighbors_ids[2] = {graph.get_number_of_nodes(), graph.get_number_of_nodes()};
    size_t edge_ids[2];
    int i = 0;
    graph.for_each_edge(colored_node_id, [&](size_t edge_id, size_t neighbor_id) {
        neighbors_ids[i] = neighbor_id;
        edge_ids[i] = edge_id;
        ++i;
    });
    if (shape.is_up(graph, edge_ids[0], neighbors_ids[0], colored_node_id)) {
        shape.remove_direction(edge_ids[0]);
        shape.set_direction(graph, edge_ids[0], colored_node_id, neighbors_ids[0], direction);
    } else {
        shape.remove_direction(edge_ids[1]);
        shape.set_direction(graph, edge_ids[1], colored_node_id, neighbors_ids[1], direction);
    }
    attributes.change_node_color(colored_node_id, dark_color);
}

void find_inconsistencies(Graph& graph, Shape& shape, GraphAttributes& attributes) {
    auto [classes_x, classes_y] = EquivalenceClasses::build(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    std::optional<Cycle> cycle_x = find_a_directed_cycle_in_graph(ordering_x);
    std::optional<Cycle> cycle_y = find_a_directed_cycle_in_graph(ordering_y);
    if (cycle_x.has_value() || cycle_y.has_value()) {
        if (cycle_x.has_value()) {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(
                graph,
                shape,
                cycle_x.value(),
                ordering_x_edge_to_graph_edge,
                false
            );
            fix_inconsistency(cycle, attributes, graph, shape, Color::BLUE);
        } else {
            Cycle cycle = build_cycle_in_graph_from_cycle_in_ordering(
                graph,
                shape,
                cycle_y.value(),
                ordering_y_edge_to_graph_edge,
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
    GraphAttributes& attributes,
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

size_t find_fixed_index_node(
    const GraphAttributes& attributes, const std::vector<size_t>& nodes_at_direction
) {
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
    GraphAttributes& attributes,
    std::vector<size_t>& nodes_at_direction,
    const Axis axis,
    const Direction increasing_direction,
    const Color color
) {
    auto position_function =
        axis == Axis::X ? [](const GraphAttributes& a, size_t id) { return a.get_position_x(id); }
                        : [](const GraphAttributes& a, size_t id) { return a.get_position_y(id); };
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
        axis == Axis::X ? [](const GraphAttributes& a, size_t id) { return a.get_position_y(id); }
                        : [](const GraphAttributes& a, size_t id) { return a.get_position_x(id); };
    const auto change_position_other =
        axis == Axis::X
            ? [](GraphAttributes& a, size_t id, int value) { a.change_position_y(id, value); }
            : [](GraphAttributes& a, size_t id, int value) { a.change_position_x(id, value); };
    size_t index_of_fixed_node = find_fixed_index_node(attributes, nodes_at_direction);
    int initial_position = position_function_other(attributes, node_id);
    graph.for_each_node([&](size_t id) {
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
    });
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
    graph.for_each_edge(node_id, [&](size_t edge_id, size_t neighbor_id) {
        Direction dir = shape.get_direction(graph, edge_id, node_id, neighbor_id);
        switch (dir) {
        case Direction::RIGHT:
            nodes_at_direction[0].push_back(neighbor_id);
            break;
        case Direction::UP:
            nodes_at_direction[1].push_back(neighbor_id);
            break;
        case Direction::LEFT:
            nodes_at_direction[2].push_back(neighbor_id);
            break;
        case Direction::DOWN:
            nodes_at_direction[3].push_back(neighbor_id);
            break;
        default:
            DOMUS_ASSERT(false, "neighbors_at_each_direction: found invalid direction");
            break;
        }
    });
    return nodes_at_direction;
}

void make_shifts_overlapped_edges(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    std::vector<size_t> nodes;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    });
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

void fix_negative_positions(const Graph& graph, GraphAttributes& attributes) {
    if (graph.get_number_of_nodes() == 0)
        return;
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    graph.for_each_node([&](size_t node_id) {
        min_x = std::min(min_x, attributes.get_position_x(node_id));
        min_y = std::min(min_y, attributes.get_position_y(node_id));
    });
    if (min_x < 0)
        graph.for_each_node([&](size_t node_id) {
            attributes.change_position_x(node_id, attributes.get_position_x(node_id) - min_x);
        });
    if (min_y < 0)
        graph.for_each_node([&](size_t node_id) {
            attributes.change_position_y(node_id, attributes.get_position_y(node_id) - min_y);
        });
}

using json = nlohmann::json;

std::expected<void, std::string>
save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result, std::filesystem::path path) {
    auto saved = save_orthogonal_drawing_to_file(result.drawing, path);
    if (!saved) {
        return std::unexpected(
            std::format("save_shape_metrics_drawing_to_file: {}", saved.error())
        );
    }
    json data;
    data["initial_number_of_cycles"] = result.initial_number_of_cycles;
    data["number_of_added_cycles"] = result.number_of_added_cycles;
    data["number_of_useless_bends"] = result.number_of_useless_bends;
    std::ofstream file(path);
    if (!file.is_open()) {
        return std::unexpected(
            std::format("save_shape_metrics_drawing_to_file: could not open {}", path.string())
        );
    }
    file << data;
    return {};
}

std::expected<ShapeMetricsDrawing, std::string>
load_shape_metrics_drawing_from_file(std::filesystem::path path) {
    auto drawing = load_orthogonal_drawing_from_file(path);
    if (!drawing) {
        return std::unexpected(
            std::format("load_shape_metrics_drawing_from_file: {}", drawing.error())
        );
    }
    std::ifstream file(path);
    json data;
    file >> data;
    ShapeMetricsDrawing result;
    result.drawing = std::move(*drawing);
    result.initial_number_of_cycles =
        static_cast<size_t>(data.value("initial_number_of_cycles", 0));
    result.number_of_added_cycles = static_cast<size_t>(data.value("number_of_added_cycles", 0));
    result.number_of_useless_bends = static_cast<size_t>(data.value("number_of_useless_bends", 0));
    return result;
}