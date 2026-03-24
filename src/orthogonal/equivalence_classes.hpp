#pragma once

#include <functional>
#include <string>
#include <utility>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/shape/shape.hpp"

namespace domus::orthogonal {
using Shape = shape::Shape;
using EdgesLabels = graph::utilities::EdgesLabels;

class EquivalenceClasses {
    graph::utilities::NodesLabels m_elem_to_class;
    std::vector<std::vector<size_t>> m_class_to_elems;
    size_t m_number_of_classes = 0;
    bool has_class(size_t class_id) const;
    void set_class(size_t elem, size_t class_id);
    size_t add_class();
    EquivalenceClasses(const domus::graph::Graph& graph);
    void directional_node_expander(
        const Shape& shape,
        const domus::graph::Graph& graph,
        size_t node_id,
        size_t class_id,
        const std::function<bool(const Shape&, size_t)>& is_direction_wrong
    );
    void
    horizontal_node_expander(const Shape& shape, const domus::graph::Graph& graph, size_t node_id);
    void
    vertical_node_expander(const Shape& shape, const domus::graph::Graph& graph, size_t node_id);

  public:
    bool has_elem_a_class(size_t elem) const;
    size_t get_class_of_elem(size_t elem) const;
    void for_each_elem_of_class(size_t class_id, std::function<void(size_t)> f) const;
    std::string to_string() const;
    void print() const;
    void for_each_class(std::function<void(size_t)> f) const;
    static const std::pair<EquivalenceClasses, EquivalenceClasses>
    build(const Shape& shape, const domus::graph::Graph& graph);
};

class Ordering {
    const domus::graph::Graph m_ordering_x;
    const domus::graph::Graph m_ordering_y;
    const EdgesLabels m_ordering_x_edge_to_graph_edge;
    const EdgesLabels m_ordering_y_edge_to_graph_edge;
    Ordering(
        const domus::graph::Graph&& ordering_x,
        const domus::graph::Graph&& ordering_y,
        const EdgesLabels&& ordering_x_edge_to_graph_edge,
        const EdgesLabels&& ordering_y_edge_to_graph_edge
    );

  public:
    const domus::graph::Graph& get_ordering_x() const;
    const domus::graph::Graph& get_ordering_y() const;
    const EdgesLabels& get_ordering_x_edge_to_graph_edge() const;
    const EdgesLabels& get_ordering_y_edge_to_graph_edge() const;

    static Ordering build(
        const EquivalenceClasses& equivalence_classes_x,
        const EquivalenceClasses& equivalence_classes_y,
        const domus::graph::Graph& graph,
        const Shape& shape
    );
};

} // namespace domus::orthogonal