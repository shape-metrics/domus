#include "domus/orthogonal/drawing_builder.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <limits.h>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/utils.hpp"
#include "domus/nlohmann/json.hpp"
#include "domus/orthogonal/area_compacter.hpp"
#include "domus/orthogonal/equivalence_classes.hpp"
#include "domus/orthogonal/shape/shape.hpp"
#include "domus/orthogonal/shape/shape_builder.hpp"

using namespace std;
using namespace std::filesystem;

vector<int> path_in_class(
    const UndirectedGraph& graph, int from, int to, const Shape& shape, bool go_horizontal
) {
    vector<int> path;
    unordered_set<int> visited;
    function<void(int)> dfs = [&](const int current) {
        if (current == to) {
            path.push_back(current);
            return;
        }
        visited.insert(current);
        for (int neighbor_id : graph.get_neighbors_of_node(current)) {
            if (visited.contains(neighbor_id))
                continue;
            if (go_horizontal == shape.is_horizontal(current, neighbor_id)) {
                dfs(neighbor_id);
                if (!path.empty()) {
                    path.push_back(current);
                    return;
                }
            }
        }
        visited.erase(current);
    };
    dfs(from);
    ranges::reverse(path);
    return path;
}

Cycle build_cycle_in_graph_from_cycle_in_ordering(
    const UndirectedGraph& graph,
    const Shape& shape,
    const Cycle& cycle_in_ordering,
    const map<pair<int, int>, pair<int, int>>& ordering_edge_to_graph_edge,
    const bool go_horizontal
) {
    vector<int> cycle;
    for (size_t i = 0; i < cycle_in_ordering.size(); ++i) {
        const int class_id = cycle_in_ordering[i];
        const int next_class_id = cycle_in_ordering.next_of_node(class_id);
        auto [from, to] = ordering_edge_to_graph_edge.at({class_id, next_class_id});
        cycle.push_back(from);
        const int next_next_class_id = cycle_in_ordering.next_of_node(next_class_id);
        auto [next_from, next_to] =
            ordering_edge_to_graph_edge.at({next_class_id, next_next_class_id});
        if (to != next_from) {
            vector<int> path = path_in_class(graph, to, next_from, shape, go_horizontal);
            const auto end = static_cast<size_t>(static_cast<int>(path.size()) - 1);
            for (size_t j = 0; j < end; ++j)
                cycle.push_back(path[j]);
        }
    }
    return Cycle(cycle);
}

// useless bends are red nodes with two horizontal or vertical edges
void remove_useless_bends(UndirectedGraph& graph, const GraphAttributes& attributes, Shape& shape) {
    vector<int> nodes_to_remove;
    for (int node_id : graph.get_nodes_ids()) {
        if (attributes.get_node_color(node_id) == Color::BLACK)
            continue;
        assert(graph.get_degree_of_node(node_id) == 2);
        array<int, 2> neighbors{-1, -1};
        size_t i = 0;
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            neighbors[i++] = neighbor_id;
        // if the added corner is flat, remove it
        if (shape.is_horizontal(node_id, neighbors[0]) ==
            shape.is_horizontal(node_id, neighbors[1]))
            nodes_to_remove.push_back(node_id);
    }
    for (int node_id : nodes_to_remove) {
        array<int, 2> neighbors{-1, -1};
        size_t i = 0;
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            neighbors[i++] = neighbor_id;
        auto direction = shape.get_direction(neighbors[0], node_id);
        graph.remove_node(node_id);
        graph.add_edge(neighbors[0], neighbors[1]);
        shape.remove_direction(node_id, neighbors[0]).value();
        shape.remove_direction(node_id, neighbors[1]).value();
        shape.remove_direction(neighbors[0], node_id).value();
        shape.remove_direction(neighbors[1], node_id).value();
        shape.set_direction(neighbors[0], neighbors[1], *direction).value();
        shape.set_direction(neighbors[1], neighbors[0], opposite_direction(*direction)).value();
    }
}

ShapeMetricsDrawing
make_orthogonal_drawing_incremental(const UndirectedGraph& graph, vector<Cycle>& cycles);

expected<ShapeMetricsDrawing, string> make_orthogonal_drawing(const UndirectedGraph& graph) {
    auto cycles = compute_cycle_basis(graph);
    if (!cycles) {
        string error = "Error in make_orthogonal_drawing:\n";
        error += cycles.error();
        return std::unexpected(error);
    }
    return make_orthogonal_drawing_incremental(graph, *cycles);
}

optional<Cycle> check_if_metrics_exist(Shape& shape, UndirectedGraph& graph) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    optional<Cycle> cycle_x = find_a_cycle_in_graph(ordering_x);
    optional<Cycle> cycle_y = find_a_cycle_in_graph(ordering_y);
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

void build_nodes_positions(UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape);

bool has_graph_degree_more_than_4(const UndirectedGraph& graph) {
    for (int node_id : graph.get_nodes_ids())
        if (graph.get_degree_of_node(node_id) > 4)
            return true;
    return false;
}

void add_green_blue_nodes(UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape);

void make_shifts_overlapped_edges(
    UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape
);

void fix_negative_positions(const UndirectedGraph& graph, GraphAttributes& attributes);

void fix_degree_more_than_4(
    UndirectedGraph& augmented_graph, GraphAttributes& attributes, Shape& shape
) {
    add_green_blue_nodes(augmented_graph, attributes, shape);
    build_nodes_positions(augmented_graph, attributes, shape);
    make_shifts_overlapped_edges(augmented_graph, attributes, shape);
    fix_negative_positions(augmented_graph, attributes);
}

ShapeMetricsDrawing
make_orthogonal_drawing_incremental(const UndirectedGraph& graph, vector<Cycle>& cycles) {
    UndirectedGraph augmented_graph;
    GraphAttributes attributes;
    attributes.add_attribute(Attribute::NODES_COLOR);
    for (const int node_id : graph.get_nodes_ids()) {
        augmented_graph.add_node(node_id);
        attributes.set_node_color(node_id, Color::BLACK);
    }
    for (int node_id : graph.get_nodes_ids())
        for (int neighbor_id : graph.get_neighbors_of_node(node_id))
            if (node_id < neighbor_id)
                augmented_graph.add_edge(node_id, neighbor_id);
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

void find_inconsistencies(UndirectedGraph& graph, Shape& shape, GraphAttributes& attributes);

void build_nodes_positions(UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape) {
    find_inconsistencies(graph, shape, attributes);
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ignored_1, ignored_2] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    auto new_classes_x_ordering = *make_topological_ordering(ordering_x);
    auto new_classes_y_ordering = *make_topological_ordering(ordering_y);
    int current_position_x = -100;
    unordered_map<int, int> node_id_to_position_x;
    for (const int class_id : new_classes_x_ordering) {
        int next_position_x = current_position_x + 100;
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::BLUE)
                next_position_x = current_position_x + 100;
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x[node_id] = next_position_x;
        current_position_x = next_position_x;
    }
    int current_position_y = -100;
    unordered_map<int, int> node_id_to_position_y;
    for (const int class_id : new_classes_y_ordering) {
        int next_position_y = current_position_y + 100;
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            if (attributes.get_node_color(node_id) == Color::GREEN)
                next_position_y = current_position_y + 100;
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y[node_id] = next_position_y;
        current_position_y = next_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (int node_id : graph.get_nodes_ids()) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    }
}

auto find_edges_to_fix(
    const UndirectedGraph& graph, const Shape& shape, const GraphAttributes& attributes
) {
    unordered_map<int, int> node_to_leftest_up;
    unordered_map<int, int> node_to_leftest_down;
    unordered_map<int, int> node_to_downest_left;
    unordered_map<int, int> node_to_downest_right;
    for (int node_id : graph.get_nodes_ids()) {
        if (graph.get_degree_of_node(node_id) <= 4)
            continue;
        optional<int> downest_left, downest_right, leftest_up, leftest_down;
        for (int added_id : graph.get_neighbors_of_node(node_id)) {
            if (shape.is_horizontal(node_id, added_id)) {
                assert(!shape.is_left(node_id, added_id));
                int other_neighbor_id = 0;
                for (int neighbor_id : graph.get_neighbors_of_node(added_id)) {
                    if (neighbor_id == node_id)
                        continue;
                    other_neighbor_id = neighbor_id;
                }
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
                int other_neighbor_id = 0;
                for (int neighbor_id : graph.get_neighbors_of_node(added_id)) {
                    if (neighbor_id == node_id)
                        continue;
                    other_neighbor_id = neighbor_id;
                }
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
        }
        node_to_leftest_up[node_id] = leftest_up.value();
        node_to_leftest_down[node_id] = leftest_down.value();
        node_to_downest_left[node_id] = downest_left.value();
        node_to_downest_right[node_id] = downest_right.value();
    }
    return std::make_tuple(
        std::move(node_to_leftest_up),
        std::move(node_to_leftest_down),
        std::move(node_to_downest_left),
        std::move(node_to_downest_right)
    );
}

int get_other_neighbor_id(const UndirectedGraph& graph, const int node_id, const int neighbor_id) {
    for (int other_id : graph.get_neighbors_of_node(node_id))
        if (other_id != neighbor_id)
            return other_id;
    assert(false); // No other neighbor found for node
    return -1;
}

void fix_edge(
    UndirectedGraph& graph,
    int node_id,
    int other_node_id,
    Shape& shape,
    GraphAttributes& attributes,
    Direction direction
) {
    int other_neighbor_id = get_other_neighbor_id(graph, other_node_id, node_id);
    graph.remove_node(other_node_id);
    attributes.remove_position(other_node_id);
    attributes.remove_nodes_attribute(other_node_id);
    graph.add_edge(node_id, other_neighbor_id);
    shape.remove_direction(node_id, other_node_id).value();
    shape.remove_direction(other_node_id, node_id).value();
    shape.remove_direction(other_node_id, other_neighbor_id).value();
    shape.remove_direction(other_neighbor_id, other_node_id).value();
    shape.set_direction(node_id, other_neighbor_id, direction).value();
    shape.set_direction(other_neighbor_id, node_id, opposite_direction(direction)).value();
}

// at the moment, a node with degree > 4 doesn't have all its "ports" used,
// this method takes some of its neighbors and places them in the unused
// "ports"
void fix_useless_green_blue_nodes(
    UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape
) {
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

void add_green_blue_nodes(UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape) {
    auto nodes_view = graph.get_nodes_ids() |
                      views::filter([&](int id) { return graph.get_degree_of_node(id) > 4; });
    vector<int> nodes(nodes_view.begin(), nodes_view.end());
    unordered_set<int> added_nodes_ids;
    for (int node_id : nodes) {
        vector<pair<int, int>> edges_to_remove;
        vector<pair<int, int>> edges_to_add;
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            int added_id = graph.add_node();
            added_nodes_ids.insert(added_id);
            edges_to_add.emplace_back(added_id, node_id);
            edges_to_add.emplace_back(added_id, neighbor_id);
            shape.set_direction(added_id, neighbor_id, *shape.get_direction(node_id, neighbor_id))
                .value();
            shape.set_direction(neighbor_id, added_id, *shape.get_direction(neighbor_id, node_id))
                .value();
            if (shape.is_horizontal(node_id, neighbor_id)) {
                attributes.set_node_color(added_id, Color::GREEN);
                shape.set_direction(node_id, added_id, Direction::UP).value();
                shape.set_direction(added_id, node_id, Direction::DOWN).value();
            } else {
                attributes.set_node_color(added_id, Color::BLUE);
                shape.set_direction(node_id, added_id, Direction::RIGHT).value();
                shape.set_direction(added_id, node_id, Direction::LEFT).value();
            }
            shape.remove_direction(node_id, neighbor_id).value();
            shape.remove_direction(neighbor_id, node_id).value();
            edges_to_remove.emplace_back(node_id, neighbor_id);
        }
        for (auto [from_id, to_id] : edges_to_add)
            graph.add_edge(from_id, to_id);
        for (auto [from_id, to_id] : edges_to_remove)
            graph.remove_edge(from_id, to_id);
    }
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto ordering = equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    DirectedGraph& ordering_x = std::get<0>(ordering);
    DirectedGraph& ordering_y = std::get<1>(ordering);
    vector<int> classes_x_ordering = *make_topological_ordering(ordering_x);
    vector<int> classes_y_ordering = *make_topological_ordering(ordering_y);
    int current_position_x = 0;
    unordered_map<int, int> node_id_to_position_x;
    for (const int class_id : classes_x_ordering) {
        for (const int node_id : classes_x.get_elems_of_class(class_id))
            node_id_to_position_x[node_id] = 100 * current_position_x;
        ++current_position_x;
    }
    int current_position_y = 0;
    unordered_map<int, int> node_id_to_position_y;
    for (const int class_id : classes_y_ordering) {
        for (const int node_id : classes_y.get_elems_of_class(class_id))
            node_id_to_position_y[node_id] = 100 * current_position_y;
        ++current_position_y;
    }
    attributes.add_attribute(Attribute::NODES_POSITION);
    for (int node_id : graph.get_nodes_ids()) {
        const int x = node_id_to_position_x[node_id];
        const int y = node_id_to_position_y[node_id];
        attributes.set_position(node_id, x, y);
    }
    fix_useless_green_blue_nodes(graph, attributes, shape);
    attributes.remove_attribute(Attribute::NODES_POSITION);
}

void fix_inconsistency(
    const Cycle& cycle,
    GraphAttributes& attributes,
    const UndirectedGraph& graph,
    Shape& shape,
    const Color color_to_find
) {
    const Direction direction = color_to_find == Color::GREEN ? Direction::UP : Direction::RIGHT;
    const Color dark_color = color_to_find == Color::GREEN ? Color::GREEN_DARK : Color::BLUE_DARK;
    optional<int> colored_node;
    for (const int node_id : cycle) {
        if (attributes.get_node_color(node_id) != color_to_find)
            continue;
        colored_node = node_id;
    }
    assert(colored_node.has_value());
    const int colored_node_id = colored_node.value();
    int neighbors_ids[2] = {-1, -1};
    int i = 0;
    for (int neighbor_id : graph.get_neighbors_of_node(colored_node_id)) {
        neighbors_ids[i] = neighbor_id;
        ++i;
    }
    if (shape.is_up(neighbors_ids[0], colored_node_id)) {
        shape.remove_direction(colored_node_id, neighbors_ids[0]).value();
        shape.remove_direction(neighbors_ids[0], colored_node_id).value();
        shape.set_direction(colored_node_id, neighbors_ids[0], direction).value();
        shape.set_direction(neighbors_ids[0], colored_node_id, opposite_direction(direction))
            .value();
    } else {
        shape.remove_direction(colored_node_id, neighbors_ids[1]).value();
        shape.remove_direction(neighbors_ids[1], colored_node_id).value();
        shape.set_direction(colored_node_id, neighbors_ids[1], direction).value();
        shape.set_direction(neighbors_ids[1], colored_node_id, opposite_direction(direction))
            .value();
    }
    attributes.change_node_color(colored_node_id, dark_color);
}

void find_inconsistencies(UndirectedGraph& graph, Shape& shape, GraphAttributes& attributes) {
    auto [classes_x, classes_y] = build_equivalence_classes(shape, graph);
    auto [ordering_x, ordering_y, ordering_x_edge_to_graph_edge, ordering_y_edge_to_graph_edge] =
        equivalence_classes_to_ordering(classes_x, classes_y, graph, shape);
    optional<Cycle> cycle_x = find_a_cycle_in_graph(ordering_x);
    optional<Cycle> cycle_y = find_a_cycle_in_graph(ordering_y);
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
    const int node_id,
    UndirectedGraph& graph,
    Shape& shape,
    vector<int>& nodes_at_direction,
    GraphAttributes& attributes,
    const Direction increasing_direction,
    Func get_position
) {
    const Direction decreasing_direction = opposite_direction(increasing_direction);
    std::sort(nodes_at_direction.begin(), nodes_at_direction.end(), [&](int a, int b) {
        if (attributes.get_node_color(a) == Color::BLACK) {
            const int b_other_neighbor_id = get_other_neighbor_id(graph, b, node_id);
            return shape.get_direction(b, b_other_neighbor_id) == increasing_direction;
        }
        if (attributes.get_node_color(b) == Color::BLACK) {
            const int a_other_neighbor_id = get_other_neighbor_id(graph, a, node_id);
            return shape.get_direction(a, a_other_neighbor_id) == decreasing_direction;
        }
        const int a_other_neighbor = get_other_neighbor_id(graph, a, node_id);
        const int b_other_neighbor = get_other_neighbor_id(graph, b, node_id);
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
find_fixed_index_node(const GraphAttributes& attributes, const vector<int>& nodes_at_direction) {
    for (size_t i = 0; i < nodes_at_direction.size(); ++i) {
        const int node_id = nodes_at_direction[i];
        if (attributes.get_node_color(node_id) == Color::BLACK)
            return i;
    }
    return nodes_at_direction.size() / 2;
}

enum class Axis { X, Y };

void make_shifts(
    const int node_id,
    UndirectedGraph& graph,
    Shape& shape,
    GraphAttributes& attributes,
    vector<int>& right_nodes,
    const Axis axis,
    const Direction increasing_direction,
    const Color color
) {
    auto position_function =
        axis == Axis::X
            ? [](const GraphAttributes& a, const int id) { return a.get_position_x(id); }
            : [](const GraphAttributes& a, const int id) { return a.get_position_y(id); };
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
        axis == Axis::X
            ? [](const GraphAttributes& a, const int id) { return a.get_position_y(id); }
            : [](const GraphAttributes& a, const int id) { return a.get_position_x(id); };
    const auto change_position_other = axis == Axis::X
                                           ? [](GraphAttributes& a,
                                                const int id,
                                                const int value) { a.change_position_y(id, value); }
                                           : [](GraphAttributes& a, const int id, const int value) {
                                                 a.change_position_x(id, value);
                                             };
    const size_t index_of_fixed_node = find_fixed_index_node(attributes, right_nodes);
    const int initial_position = position_function_other(attributes, node_id);
    for (int id : graph.get_nodes_ids()) {
        const int old_position_y = position_function_other(attributes, id);
        if (old_position_y > initial_position) {
            const auto node_count = static_cast<int>(right_nodes.size());
            const int offset = node_count - static_cast<int>(index_of_fixed_node) - 1;
            const int new_position_y = old_position_y + 5 * offset;
            change_position_other(attributes, id, new_position_y);
        }
        if (old_position_y < initial_position) {
            const int new_position_y = old_position_y - 5 * static_cast<int>(index_of_fixed_node);
            change_position_other(attributes, id, new_position_y);
        }
    }
    for (size_t i = 0; i < right_nodes.size(); ++i) {
        if (i == index_of_fixed_node)
            continue;
        const int node_to_shift_id = right_nodes[i];
        const int shift = (static_cast<int>(i) - static_cast<int>(index_of_fixed_node)) * 5;
        const int node_to_shift_neighbor_id =
            get_other_neighbor_id(graph, node_to_shift_id, node_id);
        const Direction direction =
            *shape.get_direction(node_to_shift_id, node_to_shift_neighbor_id);
        const int added_node_id = graph.add_node();
        attributes.set_node_color(added_node_id, color);
        shape.set_direction(node_id, added_node_id, direction).value();
        shape.set_direction(added_node_id, node_id, opposite_direction(direction)).value();
        shape.set_direction(added_node_id, node_to_shift_id, direction).value();
        shape.set_direction(node_to_shift_id, added_node_id, opposite_direction(direction)).value();
        shape.remove_direction(node_id, node_to_shift_id).value();
        shape.remove_direction(node_to_shift_id, node_id).value();
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

auto neighbors_at_each_direction(const UndirectedGraph& graph, int node_id, const Shape& shape) {
    unordered_map<Direction, vector<int>> nodes_at_direction;
    for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
        const Direction dir = *shape.get_direction(node_id, neighbor_id);
        nodes_at_direction[dir].push_back(neighbor_id);
    }
    return nodes_at_direction;
}

void make_shifts_overlapped_edges(
    UndirectedGraph& graph, GraphAttributes& attributes, Shape& shape
) {
    vector<int> nodes;
    for (int node_id : graph.get_nodes_ids())
        if (graph.get_degree_of_node(node_id) > 4)
            nodes.push_back(node_id);
    for (int node_id : nodes) {
        unordered_map<Direction, vector<int>> nodes_to_sort =
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

void fix_negative_positions(const UndirectedGraph& graph, GraphAttributes& attributes) {
    auto ids = graph.get_nodes_ids();
    if (ids.empty())
        return;

    int min_x =
        ranges::min(ids | views::transform([&](int id) { return attributes.get_position_x(id); }));
    int min_y =
        ranges::min(ids | views::transform([&](int id) { return attributes.get_position_y(id); }));

    if (min_x < 0)
        for (int node_id : ids)
            attributes.change_position_x(node_id, attributes.get_position_x(node_id) - min_x);
    if (min_y < 0)
        for (int node_id : ids)
            attributes.change_position_y(node_id, attributes.get_position_y(node_id) - min_y);
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