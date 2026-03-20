#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <utility>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Shape;

class EquivalenceClasses {
    NodesLabels m_elem_to_class;
    std::vector<std::vector<size_t>> m_class_to_elems;
    size_t m_number_of_classes = 0;
    bool has_class(size_t class_id) const;
    void set_class(size_t elem, size_t class_id);
    size_t add_class();
    EquivalenceClasses(const Graph& graph);
    void directional_node_expander(
        const Shape& shape,
        const Graph& graph,
        size_t node_id,
        size_t class_id,
        const std::function<bool(const Shape&, size_t)>& is_direction_wrong
    );
    void horizontal_node_expander(const Shape& shape, const Graph& graph, size_t node_id);
    void vertical_node_expander(const Shape& shape, const Graph& graph, size_t node_id);

  public:
    bool has_elem_a_class(size_t elem) const;
    size_t get_class_of_elem(size_t elem) const;
    void for_each_elem_of_class(size_t class_id, std::function<void(size_t)> f) const;
    std::string to_string() const;
    void print() const;
    void for_each_class(std::function<void(size_t)> f) const;
    static const std::pair<EquivalenceClasses, EquivalenceClasses>
    build(const Shape& shape, const Graph& graph);
};

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
);