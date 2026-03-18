#include "equivalence_classes.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/shape.hpp"

#include "../core/domus_assert.hpp"

EquivalenceClasses::EquivalenceClasses(const Graph& graph) : m_elem_to_class(graph) {}

size_t EquivalenceClasses::add_class() {
    m_class_to_elems.push_back({});
    return m_number_of_classes++;
}

bool EquivalenceClasses::has_class(size_t class_id) const { return class_id < m_number_of_classes; }

void EquivalenceClasses::set_class(size_t elem, size_t class_id) {
    DOMUS_ASSERT(
        !has_elem_a_class(elem),
        "EquivalenceClasses::set_class elem already has an assigned class"
    );
    m_elem_to_class.add_label(elem, class_id);
    m_class_to_elems.at(class_id).push_back(elem);
}

bool EquivalenceClasses::has_elem_a_class(size_t elem) const {
    return m_elem_to_class.has_label(elem);
}

size_t EquivalenceClasses::get_class_of_elem(size_t elem) const {
    DOMUS_ASSERT(
        has_elem_a_class(elem),
        "EquivalenceClasses::get_class elem does not have a class"
    );
    return m_elem_to_class.get_label(elem);
}

void EquivalenceClasses::for_each_elem_of_class(
    size_t class_id, std::function<void(size_t)> f
) const {
    DOMUS_ASSERT(has_class(class_id), "EquivalenceClasses::get_elems class does not exist");
    for (size_t elem : m_class_to_elems.at(class_id))
        f(elem);
}

void EquivalenceClasses::for_each_class(std::function<void(size_t)> f) const {
    for (size_t class_id = 0; class_id < m_number_of_classes; ++class_id)
        f(class_id);
}

void EquivalenceClasses::directional_node_expander(
    const Shape& shape,
    const Graph& graph,
    size_t node_id,
    size_t class_id,
    const std::function<bool(const Shape&, size_t, size_t)>& is_direction_wrong
) {
    set_class(node_id, class_id);
    graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
        if (has_elem_a_class(neighbor_id))
            return;
        if (is_direction_wrong(shape, node_id, neighbor_id))
            return;
        directional_node_expander(shape, graph, neighbor_id, class_id, is_direction_wrong);
    });
}

void EquivalenceClasses::horizontal_node_expander(
    const Shape& shape, const Graph& graph, size_t node_id
) {
    size_t class_id = add_class();
    auto is_direction_wrong = [](const Shape& s, size_t i, size_t j) {
        return s.is_vertical(i, j);
    };
    directional_node_expander(shape, graph, node_id, class_id, is_direction_wrong);
}

void EquivalenceClasses::vertical_node_expander(
    const Shape& shape, const Graph& graph, size_t node_id
) {
    size_t class_id = add_class();
    auto is_direction_wrong = [](const Shape& s, size_t i, size_t j) {
        return s.is_horizontal(i, j);
    };
    directional_node_expander(shape, graph, node_id, class_id, is_direction_wrong);
}

const std::pair<EquivalenceClasses, EquivalenceClasses>
EquivalenceClasses::build(const Shape& shape, const Graph& graph) {
    EquivalenceClasses equivalence_classes_x(graph);
    EquivalenceClasses equivalence_classes_y(graph);
    graph.for_each_node([&](size_t node_id) {
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            equivalence_classes_y.horizontal_node_expander(shape, graph, node_id);
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            equivalence_classes_x.vertical_node_expander(shape, graph, node_id);
    });
    graph.for_each_node([&](size_t node_id) {
        if (!equivalence_classes_x.has_elem_a_class(node_id))
            equivalence_classes_x.set_class(node_id, equivalence_classes_x.add_class());
        if (!equivalence_classes_y.has_elem_a_class(node_id))
            equivalence_classes_y.set_class(node_id, equivalence_classes_y.add_class());
    });
    return std::make_pair(std::move(equivalence_classes_x), std::move(equivalence_classes_y));
}

std::tuple<
    Graph,
    Graph,
    std::unordered_map<Edge, Edge, edge_hash>,
    std::unordered_map<Edge, Edge, edge_hash>>
equivalence_classes_to_ordering(
    const EquivalenceClasses& equivalence_classes_x,
    const EquivalenceClasses& equivalence_classes_y,
    const Graph& graph,
    const Shape& shape
) {
    Graph ordering_x;
    Graph ordering_y;

    equivalence_classes_x.for_each_class([&ordering_x](size_t) { ordering_x.add_node(); });
    equivalence_classes_y.for_each_class([&ordering_y](size_t) { ordering_y.add_node(); });
    std::unordered_map<Edge, Edge, edge_hash> ordering_x_edge_to_graph_edge;
    std::unordered_map<Edge, Edge, edge_hash> ordering_y_edge_to_graph_edge;

    graph.for_each_node([&](size_t node_id) {
        graph.for_each_neighbor(node_id, [&](size_t neighbor_id) {
            if (shape.is_right(node_id, neighbor_id)) {
                size_t node_class_x = equivalence_classes_x.get_class_of_elem(node_id);
                size_t neighbor_class_x = equivalence_classes_x.get_class_of_elem(neighbor_id);
                if (ordering_x.has_edge(node_class_x, neighbor_class_x))
                    return;
                ordering_x.add_edge(node_class_x, neighbor_class_x);
                ordering_x_edge_to_graph_edge[{node_class_x, neighbor_class_x}] = {
                    node_id,
                    neighbor_id
                };
            } else if (shape.is_up(node_id, neighbor_id)) {
                size_t node_class_y = equivalence_classes_y.get_class_of_elem(node_id);
                size_t neighbor_class_y = equivalence_classes_y.get_class_of_elem(neighbor_id);
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