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

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/nlohmann/json.hpp"
#include "domus/orthogonal/area_compacter.hpp"
#include "domus/orthogonal/equivalence_classes.hpp"
#include "domus/orthogonal/shape/shape.hpp"
#include "domus/orthogonal/shape/shape_builder.hpp"

using namespace std;
using namespace std::filesystem;

vector<size_t>
path_in_class(const Graph& graph, size_t from, size_t to, const Shape& shape, bool go_horizontal) {
    vector<size_t> path;
    NodesContainer visited;
    function<void(size_t)> dfs = [&](size_t current) {
        if (current == to) {
            path.push_back(current);
            return;
        }
        bool stop = false;
        visited.add_node(current);
        graph.for_each_neighbor(current, [&](size_t neighbor_id) {
            if (stop)
                return;
            if (visited.has_node(neighbor_id))
                return;
            if (go_horizontal == shape.is_horizontal(current, neighbor_id)) {
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
    ranges::reverse(path);
    return path;
}

Cycle build_cycle_in_graph_from_cycle_in_ordering(
    const Graph& graph,
    const Shape& shape,
    const Cycle& cycle_in_ordering,
    const unordered_map<Edge, Edge, edge_hash>& ordering_edge_to_graph_edge,
    const bool go_horizontal
) {
    vector<size_t> cycle;
    for (size_t i = 0; i < cycle_in_ordering.size(); ++i) {
        size_t class_id = cycle_in_ordering[i];
        size_t next_class_id = cycle_in_ordering.next_of_node(class_id);
        auto [from, to] = ordering_edge_to_graph_edge.at({class_id, next_class_id});
        cycle.push_back(from);
        size_t next_next_class_id = cycle_in_ordering.next_of_node(next_class_id);
        auto [next_from, next_to] =
            ordering_edge_to_graph_edge.at({next_class_id, next_next_class_id});
        if (to != next_from) {
            vector<size_t> path = path_in_class(graph, to, next_from, shape, go_horizontal);
            const auto end = static_cast<size_t>(static_cast<int>(path.size()) - 1);
            for (size_t j = 0; j < end; ++j)
                cycle.push_back(path[j]);
        }
    }
    return Cycle(cycle);
}

// useless bends are red nodes with two horizontal or vertical edges
void remove_useless_bends(Graph& graph, const GraphAttributes& attributes, Shape& shape) {
    vector<size_t> nodes_to_remove;
    graph.for_each_node([&](size_t node_id) {
        if (attributes.get_node_color(node_id) == Color::BLACK)
            return;
        assert(graph.get_degree_of_node(node_id) == 2);
        array<size_t, 2> neighbors{graph.size(), graph.size()};
        size_t i = 0;
        graph.for_each_neighbor(node_id, [&neighbors, &i](size_t neighbor_id) {
            neighbors[i++] = neighbor_id;
        });
        // if the added corner is flat, remove it
        if (shape.is_horizontal(node_id, neighbors[0]) ==
            shape.is_horizontal(node_id, neighbors[1]))
            nodes_to_remove.push_back(node_id);
    });
    for (size_t node_id : nodes_to_remove) {
        array<size_t, 2> neighbors{graph.size(), graph.size()};
        size_t i = 0;
        graph.for_each_neighbor(node_id, [&neighbors, &i](size_t neighbor_id) {
            neighbors[i++] = neighbor_id;
        });
        auto direction = shape.get_direction(neighbors[0], node_id);
        graph.remove_node(node_id);
        graph.add_edge(neighbors[0], neighbors[1]);
        shape.remove_direction(node_id, neighbors[0]);
        shape.remove_direction(node_id, neighbors[1]);
        shape.remove_direction(neighbors[0], node_id);
        shape.remove_direction(neighbors[1], node_id);
        shape.set_direction(neighbors[0], neighbors[1], *direction);
        shape.set_direction(neighbors[1], neighbors[0], opposite_direction(*direction));
    }
}

ShapeMetricsDrawing make_orthogonal_drawing_incremental(const Graph& graph, vector<Cycle>& cycles);

expected<ShapeMetricsDrawing, string> make_orthogonal_drawing(const Graph& graph) {
    auto cycles = compute_cycle_basis(graph);
    return make_orthogonal_drawing_incremental(graph, cycles);
}

optional<Cycle> check_if_metrics_exist(Shape& shape, Graph& graph) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    optional<Cycle> cycle_x = find_a_directed_cycle_in_graph(ordering_x);
    optional<Cycle> cycle_y = find_a_directed_cycle_in_graph(ordering_y);
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

void fix_degree_more_than_4(Graph& augmented_graph, GraphAttributes& attributes, Shape& shape) {
    add_green_blue_nodes(augmented_graph, attributes, shape);
    build_nodes_positions(augmented_graph, attributes, shape);
    make_shifts_overlapped_edges(augmented_graph, attributes, shape);
    fix_negative_positions(augmented_graph, attributes);
}

ShapeMetricsDrawing make_orthogonal_drawing_incremental(const Graph& graph, vector<Cycle>& cycles) {
    Graph augmented_graph;
    GraphAttributes attributes;
    attributes.add_attribute(Attribute::NODES_COLOR);
    graph.for_each_node([&](size_t node_id) {
        augmented_graph.add_node(node_id);
        attributes.set_node_color(node_id, Color::BLACK);
    });
    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (node_id < neighbor_id)
                augmented_graph.add_edge(node_id, neighbor_id);
        });
    });
    Shape shape = build_shape(augmented_graph, attributes, cycles);
    optional<Cycle> cycle_to_add = check_if_metrics_exist(shape, augmented_graph);
    size_t number_of_added_cycles = 0;
    while (cycle_to_add.has_value()) {
        cycles.push_back(std::move(*cycle_to_add));
        number_of_added_cycles++;
        shape = build_shape(augmented_graph, attributes, cycles);
        cycle_to_add = check_if_metrics_exist(shape, augmented_graph);
    }
    const size_t old_size = augmented_graph.size();
    remove_useless_bends(augmented_graph, attributes, shape);
    // from now on cycles are not valid anymore
    const size_t number_of_cycles = cycles.size();
    cycles.clear();
    const size_t number_of_useless_bends = old_size - augmented_graph.size();
    if (has_graph_degree_more_than_4(augmented_graph)) {
        fix_degree_more_than_4(augmented_graph, attributes, shape);
    } else {
        build_nodes_positions(augmented_graph, attributes, shape);
    }
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
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ignored_1, ignored_2] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    auto new_classes_x_ordering = *make_topological_ordering(ordering_x);
    auto new_classes_y_ordering = *make_topological_ordering(ordering_y);
    int current_position_x = -100;
    unordered_map<size_t, int> node_id_to_position_x;
    for (size_t class_id : new_classes_x_ordering) {
        int next_position_x = current_position_x + 100;

        classes_x.get_elems_of_class(class_id).for_each([&](size_t node_id) {
            if (attributes.get_node_color(node_id) == Color::BLUE)
                next_position_x = current_position_x + 100;
        });
        classes_x.get_elems_of_class(class_id).for_each([&](size_t node_id) {
            node_id_to_position_x[node_id] = next_position_x;
        });
        current_position_x = next_position_x;
    }
    int current_position_y = -100;
    unordered_map<size_t, int> node_id_to_position_y;
    for (size_t class_id : new_classes_y_ordering) {
        int next_position_y = current_position_y + 100;
        classes_y.get_elems_of_class(class_id).for_each([&](size_t node_id) {
            if (attributes.get_node_color(node_id) == Color::GREEN)
                next_position_y = current_position_y + 100;
        });
        classes_y.get_elems_of_class(class_id).for_each([&](size_t node_id) {
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

auto find_edges_to_fix(const Graph& graph, const Shape& shape, const GraphAttributes& attributes) {
    unordered_map<size_t, size_t> node_to_leftest_up;
    unordered_map<size_t, size_t> node_to_leftest_down;
    unordered_map<size_t, size_t> node_to_downest_left;
    unordered_map<size_t, size_t> node_to_downest_right;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) <= 4)
            return;
        optional<size_t> downest_left, downest_right, leftest_up, leftest_down;
        graph.for_each_neighbor(node_id, [&](size_t added_id) {
            if (shape.is_horizontal(node_id, added_id)) {
                assert(!shape.is_left(node_id, added_id));
                size_t other_neighbor_id = 0;
                graph.for_each_neighbor(added_id, [&](size_t neighbor_id) {
                    if (neighbor_id == node_id)
                        return;
                    other_neighbor_id = neighbor_id;
                });
                if (shape.is_up(added_id, other_neighbor_id)) {
                    if (!leftest_up.has_value())
                        leftest_up = added_id;
                    else if (attributes.get_position_x(added_id) <
                             attributes.get_position_x(leftest_up.value()))
                        leftest_up = added_id;
                } else {
                    if (!leftest_down.has_value())
                        leftest_down = added_id;
                    else if (attributes.get_position_x(added_id) <
                             attributes.get_position_x(leftest_down.value()))
                        leftest_down = added_id;
                }
            } else {
                assert(!shape.is_down(node_id, added_id));
                size_t other_neighbor_id = 0;
                graph.for_each_neighbor(added_id, [&](size_t neighbor_id) {
                    if (neighbor_id == node_id)
                        return;
                    other_neighbor_id = neighbor_id;
                });
                if (shape.is_left(added_id, other_neighbor_id)) {
                    if (!downest_left.has_value())
                        downest_left = added_id;
                    else if (attributes.get_position_y(added_id) <
                             attributes.get_position_y(downest_left.value()))
                        downest_left = added_id;
                } else {
                    if (!downest_right.has_value())
                        downest_right = added_id;
                    else if (attributes.get_position_y(added_id) <
                             attributes.get_position_y(downest_right.value())) {
                        downest_right = added_id;
                    }
                }
            }
        });
        node_to_leftest_up[node_id] = leftest_up.value();
        node_to_leftest_down[node_id] = leftest_down.value();
        node_to_downest_left[node_id] = downest_left.value();
        node_to_downest_right[node_id] = downest_right.value();
    });
    return std::make_tuple(
        std::move(node_to_leftest_up),
        std::move(node_to_leftest_down),
        std::move(node_to_downest_left),
        std::move(node_to_downest_right)
    );
}

size_t get_other_neighbor_id(const Graph& graph, size_t node_id, size_t neighbor_id) {
    optional<size_t> other;
    graph.for_each_neighbor(node_id, [&](size_t other_id) {
        if (other.has_value())
            return;
        if (other_id != neighbor_id)
            other = other_id;
    });
    assert(other.has_value()); // No other neighbor found for node
    return other.value();
}

void fix_edge(
    Graph& graph,
    size_t node_id,
    size_t other_node_id,
    Shape& shape,
    GraphAttributes& attributes,
    Direction direction
) {
    size_t other_neighbor_id = get_other_neighbor_id(graph, other_node_id, node_id);
    graph.remove_node(other_node_id);
    attributes.remove_position(other_node_id);
    attributes.remove_nodes_attribute(other_node_id);
    graph.add_edge(node_id, other_neighbor_id);
    shape.remove_direction(node_id, other_node_id);
    shape.remove_direction(other_node_id, node_id);
    shape.remove_direction(other_node_id, other_neighbor_id);
    shape.remove_direction(other_neighbor_id, other_node_id);
    shape.set_direction(node_id, other_neighbor_id, direction);
    shape.set_direction(other_neighbor_id, node_id, opposite_direction(direction));
}

// at the moment, a node with degree > 4 doesn't have all its "ports" used,
// this method takes some of its neighbors and places them in the unused
// "ports"
void fix_useless_green_blue_nodes(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    auto [node_to_leftest_up, node_to_leftest_down, node_to_downest_left, node_to_downest_right] =
        find_edges_to_fix(graph, shape, attributes);
    for (auto [node, leftest_up] : node_to_leftest_up)
        fix_edge(graph, node, leftest_up, shape, attributes, Direction::UP);
    for (auto [node, leftest_down] : node_to_leftest_down)
        fix_edge(graph, node, leftest_down, shape, attributes, Direction::DOWN);
    for (auto [node, downest_left] : node_to_downest_left)
        fix_edge(graph, node, downest_left, shape, attributes, Direction::LEFT);
    for (auto [node, downest_right] : node_to_downest_right)
        fix_edge(graph, node, downest_right, shape, attributes, Direction::RIGHT);
}

void add_green_blue_nodes(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    vector<size_t> nodes;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    });
    for (size_t node_id : nodes) {
        vector<pair<size_t, size_t>> edges_to_remove;
        vector<pair<size_t, size_t>> edges_to_add;
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            size_t added_id = graph.add_node();
            edges_to_add.emplace_back(added_id, node_id);
            edges_to_add.emplace_back(added_id, neighbor_id);
            shape.set_direction(added_id, neighbor_id, *shape.get_direction(node_id, neighbor_id));
            shape.set_direction(neighbor_id, added_id, *shape.get_direction(neighbor_id, node_id));
            if (shape.is_horizontal(node_id, neighbor_id)) {
                attributes.set_node_color(added_id, Color::GREEN);
                shape.set_direction(node_id, added_id, Direction::UP);
                shape.set_direction(added_id, node_id, Direction::DOWN);
            } else {
                attributes.set_node_color(added_id, Color::BLUE);
                shape.set_direction(node_id, added_id, Direction::RIGHT);
                shape.set_direction(added_id, node_id, Direction::LEFT);
            }
            shape.remove_direction(node_id, neighbor_id);
            shape.remove_direction(neighbor_id, node_id);
            edges_to_remove.emplace_back(node_id, neighbor_id);
        });
        for (auto [from_id, to_id] : edges_to_add)
            graph.add_edge(from_id, to_id);
        for (auto [from_id, to_id] : edges_to_remove)
            graph.remove_edge(from_id, to_id);
    }
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto ordering = equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    Graph& ordering_x = std::get<0>(ordering);
    Graph& ordering_y = std::get<1>(ordering);
    vector<size_t> classes_x_ordering = *make_topological_ordering(ordering_x);
    vector<size_t> classes_y_ordering = *make_topological_ordering(ordering_y);
    int current_position_x = 0;
    unordered_map<size_t, int> node_id_to_position_x;
    for (size_t class_id : classes_x_ordering) {
        classes_x.get_elems_of_class(class_id).for_each([&](size_t node_id) {
            node_id_to_position_x[node_id] = 100 * current_position_x;
        });
        ++current_position_x;
    }
    int current_position_y = 0;
    unordered_map<size_t, int> node_id_to_position_y;
    for (size_t class_id : classes_y_ordering) {
        classes_y.get_elems_of_class(class_id).for_each([&](size_t node_id) {
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
    fix_useless_green_blue_nodes(graph, attributes, shape);
    attributes.remove_attribute(Attribute::NODES_POSITION);
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
    optional<size_t> colored_node;
    cycle.for_each([&](size_t node_id) {
        if (attributes.get_node_color(node_id) != color_to_find)
            return;
        colored_node = node_id;
    });
    assert(colored_node.has_value());
    size_t colored_node_id = colored_node.value();
    size_t neighbors_ids[2] = {graph.size(), graph.size()};
    int i = 0;
    graph.for_each_neighbor(colored_node_id, [&](size_t neighbor_id) {
        neighbors_ids[i] = neighbor_id;
        ++i;
    });
    if (shape.is_up(neighbors_ids[0], colored_node_id)) {
        shape.remove_direction(colored_node_id, neighbors_ids[0]);
        shape.remove_direction(neighbors_ids[0], colored_node_id);
        shape.set_direction(colored_node_id, neighbors_ids[0], direction);
        shape.set_direction(neighbors_ids[0], colored_node_id, opposite_direction(direction));
    } else {
        shape.remove_direction(colored_node_id, neighbors_ids[1]);
        shape.remove_direction(neighbors_ids[1], colored_node_id);
        shape.set_direction(colored_node_id, neighbors_ids[1], direction);
        shape.set_direction(neighbors_ids[1], colored_node_id, opposite_direction(direction));
    }
    attributes.change_node_color(colored_node_id, dark_color);
}

void find_inconsistencies(Graph& graph, Shape& shape, GraphAttributes& attributes) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    optional<Cycle> cycle_x = find_a_directed_cycle_in_graph(ordering_x);
    optional<Cycle> cycle_y = find_a_directed_cycle_in_graph(ordering_y);
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
    vector<size_t>& nodes_at_direction,
    GraphAttributes& attributes,
    const Direction increasing_direction,
    Func get_position
) {
    const Direction decreasing_direction = opposite_direction(increasing_direction);
    std::sort(nodes_at_direction.begin(), nodes_at_direction.end(), [&](size_t a, size_t b) {
        if (attributes.get_node_color(a) == Color::BLACK) {
            size_t b_other_neighbor_id = get_other_neighbor_id(graph, b, node_id);
            return shape.get_direction(b, b_other_neighbor_id) == increasing_direction;
        }
        if (attributes.get_node_color(b) == Color::BLACK) {
            size_t a_other_neighbor_id = get_other_neighbor_id(graph, a, node_id);
            return shape.get_direction(a, a_other_neighbor_id) == decreasing_direction;
        }
        size_t a_other_neighbor = get_other_neighbor_id(graph, a, node_id);
        size_t b_other_neighbor = get_other_neighbor_id(graph, b, node_id);
        if (shape.get_direction(a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(b, b_other_neighbor) == decreasing_direction) {
            return false;
        }
        if (shape.get_direction(a, a_other_neighbor) == decreasing_direction &&
            shape.get_direction(b, b_other_neighbor) == increasing_direction) {
            return true;
        }
        if (shape.get_direction(a, a_other_neighbor) == increasing_direction &&
            shape.get_direction(b, b_other_neighbor) == increasing_direction) {
            return get_position(attributes, a) > get_position(attributes, b);
        }
        return get_position(attributes, a) < get_position(attributes, b);
    });
}

size_t
find_fixed_index_node(const GraphAttributes& attributes, const vector<size_t>& nodes_at_direction) {
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
    vector<size_t>& right_nodes,
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
        right_nodes,
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
    size_t index_of_fixed_node = find_fixed_index_node(attributes, right_nodes);
    int initial_position = position_function_other(attributes, node_id);
    graph.for_each_node([&](size_t id) {
        int old_position_y = position_function_other(attributes, id);
        if (old_position_y > initial_position) {
            int node_count = static_cast<int>(right_nodes.size());
            int offset = node_count - static_cast<int>(index_of_fixed_node) - 1;
            int new_position_y = old_position_y + 5 * offset;
            change_position_other(attributes, id, new_position_y);
        }
        if (old_position_y < initial_position) {
            const int new_position_y = old_position_y - 5 * static_cast<int>(index_of_fixed_node);
            change_position_other(attributes, id, new_position_y);
        }
    });
    for (size_t i = 0; i < right_nodes.size(); ++i) {
        if (i == index_of_fixed_node)
            continue;
        size_t node_to_shift_id = right_nodes[i];
        int shift = (static_cast<int>(i) - static_cast<int>(index_of_fixed_node)) * 5;
        size_t node_to_shift_neighbor_id = get_other_neighbor_id(graph, node_to_shift_id, node_id);
        Direction direction = *shape.get_direction(node_to_shift_id, node_to_shift_neighbor_id);
        size_t added_node_id = graph.add_node();
        attributes.set_node_color(added_node_id, color);
        shape.set_direction(node_id, added_node_id, direction);
        shape.set_direction(added_node_id, node_id, opposite_direction(direction));
        shape.set_direction(added_node_id, node_to_shift_id, direction);
        shape.set_direction(node_to_shift_id, added_node_id, opposite_direction(direction));
        shape.remove_direction(node_id, node_to_shift_id);
        shape.remove_direction(node_to_shift_id, node_id);
        graph.remove_edge(node_id, node_to_shift_id);
        graph.add_edge(node_id, added_node_id);
        graph.add_edge(added_node_id, node_to_shift_id);
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
    unordered_map<Direction, vector<size_t>> nodes_at_direction;
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        Direction dir = *shape.get_direction(node_id, neighbor_id);
        nodes_at_direction[dir].push_back(neighbor_id);
    });
    return nodes_at_direction;
}

void make_shifts_overlapped_edges(Graph& graph, GraphAttributes& attributes, Shape& shape) {
    vector<size_t> nodes;
    graph.for_each_node([&](size_t node_id) {
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    });
    for (size_t node_id : nodes) {
        unordered_map<Direction, vector<size_t>> nodes_to_sort =
            neighbors_at_each_direction(graph, node_id, shape);
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[Direction::RIGHT],
            Axis::X,
            Direction::UP,
            Color::GREEN
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[Direction::UP],
            Axis::Y,
            Direction::RIGHT,
            Color::BLUE
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[Direction::LEFT],
            Axis::X,
            Direction::UP,
            Color::GREEN_DARK
        );
        make_shifts(
            node_id,
            graph,
            shape,
            attributes,
            nodes_to_sort[Direction::DOWN],
            Axis::Y,
            Direction::RIGHT,
            Color::BLUE_DARK
        );
    }
}

void fix_negative_positions(const Graph& graph, GraphAttributes& attributes) {
    if (graph.size() == 0)
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

expected<void, string>
save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result, path path) {
    auto saved = save_orthogonal_drawing_to_file(result.drawing, path);
    if (!saved) {
        string error_msg = "Error in save_shape_metrics_drawing_to_file: " + saved.error();
        return std::unexpected(error_msg);
    }
    json data;
    data["initial_number_of_cycles"] = result.initial_number_of_cycles;
    data["number_of_added_cycles"] = result.number_of_added_cycles;
    data["number_of_useless_bends"] = result.number_of_useless_bends;
    std::ofstream file(path);
    if (!file.is_open()) {
        string error_msg = "Error in save_shape_metrics_drawing_to_file: " + path.string();
        error_msg += " could not be opened";
        return std::unexpected(error_msg);
    }
    file << data;
    return {};
}

expected<ShapeMetricsDrawing, string> load_shape_metrics_drawing_from_file(path path) {
    auto drawing = load_orthogonal_drawing_from_file(path);
    if (!drawing) {
        string error_msg = "Error in load_shape_metrics_drawing_from_file: " + drawing.error();
        return std::unexpected(error_msg);
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