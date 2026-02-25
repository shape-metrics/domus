#include "domus/orthogonal/equivalence_classes.hpp"

#include <cassert>
#include <functional>
#include <map>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/orthogonal/shape/shape.hpp"

using namespace std;

bool EquivalenceClasses::has_class(int class_id) const { return m_all_classes.has(class_id); }

void EquivalenceClasses::set_class(int elem, int class_id) {
    assert(
        !has_elem_a_class(elem) &&
        "EquivalenceClasses::set_class elem already has an assigned class"
    );
    m_elem_to_class.add(elem, class_id);
    m_all_classes.add(class_id);
    m_class_to_elems.add(class_id, elem);
}

bool EquivalenceClasses::has_elem_a_class(int elem) const { return m_elem_to_class.has(elem); }

int EquivalenceClasses::get_class_of_elem(int elem) const {
    assert(has_elem_a_class(elem) && "EquivalenceClasses::get_class elem does not have a class");
    return m_elem_to_class.get(elem);
}

const IntHashSet& EquivalenceClasses::get_elems_of_class(int class_id) const {
    assert(has_class(class_id) && "EquivalenceClasses::get_elems class does not exist");
    return m_class_to_elems.get(class_id);
}

const IntHashSet& EquivalenceClasses::get_all_classes() const { return m_all_classes; }

void directional_node_expander(
    const Shape& shape,
    const UndirectedGraph& graph,
    int node_id,
    int class_id,
    EquivalenceClasses& equivalence_classes,
    const function<bool(const Shape&, int, int)>& is_direction_wrong
) {
    equivalence_classes.set_class(node_id, class_id);
    graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
        if (equivalence_classes.has_elem_a_class(neighbor_id))
            return;
        if (is_direction_wrong(shape, node_id, neighbor_id))
            return;
        directional_node_expander(
            shape,
            graph,
            neighbor_id,
            class_id,
            equivalence_classes,
            is_direction_wrong
        );
    });
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
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            horizontal_node_expander(shape, graph, node_id, next_class_y++, equivalence_classes_y);
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            vertical_node_expander(shape, graph, node_id, next_class_x++, equivalence_classes_x);
    });
    graph.get_nodes_ids().for_each([&](int node_id) {
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            equivalence_classes_x.set_class(node_id, next_class_x++);
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            equivalence_classes_y.set_class(node_id, next_class_y++);
    });
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

    equivalence_classes_x.get_all_classes().for_each([&ordering_x](int class_id) {
        ordering_x.add_node(class_id);
    });
    equivalence_classes_y.get_all_classes().for_each([&ordering_y](int class_id) {
        ordering_y.add_node(class_id);
    });
    map<pair<int, int>, pair<int, int>> ordering_x_edge_to_graph_edge;
    map<pair<int, int>, pair<int, int>> ordering_y_edge_to_graph_edge;

    graph.get_nodes_ids().for_each([&](int node_id) {
        graph.get_neighbors_of_node(node_id).for_each([&](int neighbor_id) {
            if (shape.is_right(node_id, neighbor_id)) {
                const int node_class_x = equivalence_classes_x.get_class_of_elem(node_id);
                const int neighbor_class_x = equivalence_classes_x.get_class_of_elem(neighbor_id);
                if (ordering_x.has_edge(node_class_x, neighbor_class_x))
                    return;
                ordering_x.add_edge(node_class_x, neighbor_class_x);
                ordering_x_edge_to_graph_edge[{node_class_x, neighbor_class_x}] = {
                    node_id,
                    neighbor_id
                };
            } else if (shape.is_up(node_id, neighbor_id)) {
                const int node_class_y = equivalence_classes_y.get_class_of_elem(node_id);
                const int neighbor_class_y = equivalence_classes_y.get_class_of_elem(neighbor_id);
                if (ordering_y.has_edge(node_class_y, neighbor_class_y))
                    return;
                ordering_y.add_edge(node_class_y, neighbor_class_y);
                ordering_y_edge_to_graph_edge[{node_class_y, neighbor_class_y}] = {
                    node_id,
                    neighbor_id
                };
            }
        });
    });
    return make_tuple(
        std::move(ordering_x),
        std::move(ordering_y),
        std::move(ordering_x_edge_to_graph_edge),
        std::move(ordering_y_edge_to_graph_edge)
    );
}