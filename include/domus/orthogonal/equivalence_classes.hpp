#pragma once

#include <functional>
#include <string>
#include <utility>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/shape.hpp"

namespace domus::orthogonal {

class EquivalenceClasses {
    graph::utilities::NodesLabels<size_t> m_elem_to_class;
    std::vector<std::vector<size_t>> m_class_to_elems;
    size_t m_number_of_classes = 0;
    bool has_class(size_t class_id) const;
    void set_class(size_t elem, size_t class_id);
    size_t add_class();
    EquivalenceClasses(const domus::graph::Graph& graph);
    void directional_node_expander(
        const shape::Shape& shape,
        const graph::Graph& graph,
        size_t node_id,
        size_t class_id,
        const std::function<bool(const shape::Shape&, size_t)>& is_direction_wrong
    );
    void
    horizontal_node_expander(const shape::Shape& shape, const graph::Graph& graph, size_t node_id);
    void
    vertical_node_expander(const shape::Shape& shape, const graph::Graph& graph, size_t node_id);

  public:
    bool has_elem_a_class(size_t elem) const;
    size_t get_class_of_elem(size_t elem) const;
    auto get_elems_of_class(size_t class_id) const;
    size_t number_of_elems_in_class(size_t class_id) const;

    std::string to_string() const;
    void print() const;

    auto get_classes() const;

    static const std::pair<EquivalenceClasses, EquivalenceClasses>
    build(const shape::Shape& shape, const graph::Graph& graph);
};

inline auto EquivalenceClasses::get_classes() const {
    return std::views::iota(size_t{0}, m_number_of_classes);
}

inline auto EquivalenceClasses::get_elems_of_class(size_t class_id) const {
    return std::views::all(m_class_to_elems[class_id]);
}

class Ordering {
    const graph::Graph m_ordering_x;
    const graph::Graph m_ordering_y;
    const graph::utilities::EdgesLabels<size_t> m_ordering_x_edge_to_graph_edge;
    const graph::utilities::EdgesLabels<size_t> m_ordering_y_edge_to_graph_edge;
    Ordering(
        const graph::Graph&& ordering_x,
        const graph::Graph&& ordering_y,
        const graph::utilities::EdgesLabels<size_t>&& ordering_x_edge_to_graph_edge,
        const graph::utilities::EdgesLabels<size_t>&& ordering_y_edge_to_graph_edge
    );

  public:
    const graph::Graph& get_ordering_x() const;
    const graph::Graph& get_ordering_y() const;
    const graph::utilities::EdgesLabels<size_t>& get_ordering_x_edge_to_graph_edge() const;
    const graph::utilities::EdgesLabels<size_t>& get_ordering_y_edge_to_graph_edge() const;

    static Ordering build(
        const EquivalenceClasses& equivalence_classes_x,
        const EquivalenceClasses& equivalence_classes_y,
        const graph::Graph& graph,
        const shape::Shape& shape
    );
};

} // namespace domus::orthogonal