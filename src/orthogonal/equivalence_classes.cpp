#include "domus/orthogonal/equivalence_classes.hpp"

#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

#include "domus/orthogonal/shape/shape.hpp"

using namespace std;

bool EquivalenceClasses::has_class(int class_id) const {
    return m_class_to_elems.contains(class_id);
}

void EquivalenceClasses::set_class(int elem, int class_id) {
    if (has_elem_a_class(elem))
        throw runtime_error("EquivalenceClasses::set_class elem already has an assigned class");
    m_elem_to_class[elem] = class_id;
    m_class_to_elems[class_id].insert(elem);
}

bool EquivalenceClasses::has_elem_a_class(int elem) const { return m_elem_to_class.contains(elem); }

int EquivalenceClasses::get_class_of_elem(int elem) const {
    if (!has_elem_a_class(elem))
        throw runtime_error("EquivalenceClasses::get_class elem does not have a class");
    return m_elem_to_class.at(elem);
}

const unordered_set<int>& EquivalenceClasses::get_elems_of_class(int class_id) const {
    if (!has_class(class_id))
        throw runtime_error("EquivalenceClasses::get_elems class does not exist");
    return m_class_to_elems.at(class_id);
}

string EquivalenceClasses::to_string() const {
    string result = "EquivalenceClasses:\n";
    for (const auto& [fst, snd] : m_class_to_elems) {
        result += "Class " + std::to_string(fst) + ": ";
        for (const int elem : snd)
            result += std::to_string(elem) + " ";
        result += "\n";
    }
    return result;
}

void EquivalenceClasses::print() const { std::cout << to_string() << std::endl; }

void directional_node_expander(
    const Shape& shape,
    const UndirectedGraph& graph,
    int node_id,
    int class_id,
    EquivalenceClasses& equivalence_classes,
    const function<bool(const Shape&, int, int)>& is_direction_wrong
) {
    equivalence_classes.set_class(node_id, class_id);
    for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
        if (equivalence_classes.has_elem_a_class(neighbor_id))
            continue;
        if (is_direction_wrong(shape, node_id, neighbor_id))
            continue;
        directional_node_expander(
            shape,
            graph,
            neighbor_id,
            class_id,
            equivalence_classes,
            is_direction_wrong
        );
    }
}

void horizontal_node_expander(
    const Shape& shape,
    const UndirectedGraph& graph,
    int node_id,
    const int class_id,
    EquivalenceClasses& equivalence_classes
) {
    auto is_direction_wrong = [](const Shape& s, int i, int j) { return s.is_vertical(i, j); };
    directional_node_expander(
        shape,
        graph,
        node_id,
        class_id,
        equivalence_classes,
        is_direction_wrong
    );
}

void vertical_node_expander(
    const Shape& shape,
    const UndirectedGraph& graph,
    int node_id,
    const int class_id,
    EquivalenceClasses& equivalence_classes
) {
    auto is_direction_wrong = [](const Shape& s, int i, int j) { return s.is_horizontal(i, j); };
    directional_node_expander(
        shape,
        graph,
        node_id,
        class_id,
        equivalence_classes,
        is_direction_wrong
    );
}

pair<const EquivalenceClasses, const EquivalenceClasses>
build_equivalence_classes(const Shape& shape, const UndirectedGraph& graph) {
    EquivalenceClasses equivalence_classes_x;
    EquivalenceClasses equivalence_classes_y;
    int next_class_x = 0;
    int next_class_y = 0;
    for (int node_id : graph.get_nodes_ids()) {
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            horizontal_node_expander(shape, graph, node_id, next_class_y++, equivalence_classes_y);
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            vertical_node_expander(shape, graph, node_id, next_class_x++, equivalence_classes_x);
    }
    for (int node_id : graph.get_nodes_ids()) {
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            equivalence_classes_x.set_class(node_id, next_class_x++);
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            equivalence_classes_y.set_class(node_id, next_class_y++);
    }
    return make_pair(std::move(equivalence_classes_x), std::move(equivalence_classes_y));
}

tuple<
    DirectedGraph,
    DirectedGraph,
    map<pair<int, int>, pair<int, int>>,
    map<pair<int, int>, pair<int, int>>>
equivalence_classes_to_ordering(
    const EquivalenceClasses& equivalence_classes_x,
    const EquivalenceClasses& equivalence_classes_y,
    const UndirectedGraph& graph,
    const Shape& shape
) {
    DirectedGraph ordering_x;
    DirectedGraph ordering_y;
    for (const int class_id : equivalence_classes_x.get_all_classes())
        ordering_x.add_node(class_id);
    for (const int class_id : equivalence_classes_y.get_all_classes())
        ordering_y.add_node(class_id);
    map<pair<int, int>, pair<int, int>> ordering_x_edge_to_graph_edge;
    map<pair<int, int>, pair<int, int>> ordering_y_edge_to_graph_edge;
    for (int node_id : graph.get_nodes_ids()) {
        for (int neighbor_id : graph.get_neighbors_of_node(node_id)) {
            if (shape.is_right(node_id, neighbor_id)) {
                const int node_class_x = equivalence_classes_x.get_class_of_elem(node_id);
                const int neighbor_class_x = equivalence_classes_x.get_class_of_elem(neighbor_id);
                if (ordering_x.has_edge(node_class_x, neighbor_class_x))
                    continue;
                ordering_x.add_edge(node_class_x, neighbor_class_x);
                ordering_x_edge_to_graph_edge[{node_class_x, neighbor_class_x}] = {
                    node_id,
                    neighbor_id
                };
            } else if (shape.is_up(node_id, neighbor_id)) {
                const int node_class_y = equivalence_classes_y.get_class_of_elem(node_id);
                const int neighbor_class_y = equivalence_classes_y.get_class_of_elem(neighbor_id);
                if (ordering_y.has_edge(node_class_y, neighbor_class_y))
                    continue;
                ordering_y.add_edge(node_class_y, neighbor_class_y);
                ordering_y_edge_to_graph_edge[{node_class_y, neighbor_class_y}] = {
                    node_id,
                    neighbor_id
                };
            }
        }
    }
    return make_tuple(
        std::move(ordering_x),
        std::move(ordering_y),
        std::move(ordering_x_edge_to_graph_edge),
        std::move(ordering_y_edge_to_graph_edge)
    );
}