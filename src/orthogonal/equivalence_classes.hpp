#pragma once

#include <string>
#include <tuple>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Shape;

class EquivalenceClasses {
    Int_ToInt_HashMap m_elem_to_class;
    Int_ToIntContainer_HashMap m_class_to_elems;
    IntHashSet m_all_classes;
    bool has_class(size_t class_id) const;

  public:
    void set_class(size_t elem, size_t class_id);
    bool has_elem_a_class(size_t elem) const;
    size_t get_class_of_elem(size_t elem) const;
    const IntHashSet& get_elems_of_class(size_t class_id) const;
    std::string to_string() const;
    void print() const;
    const IntHashSet& get_all_classes() const;
};

std::pair<const EquivalenceClasses, const EquivalenceClasses>
build_equivalence_classes(const Shape& shape, const Graph& graph);

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