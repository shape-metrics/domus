#ifndef MY_EUIVALENCE_CLASSES_H
#define MY_EUIVALENCE_CLASSES_H

#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/core/graph/graph.hpp"

class Shape;

class EquivalenceClasses {
    Int_ToInt_HashMap m_elem_to_class;
    Int_ToIntContainer_HashMap m_class_to_elems;
    IntHashSet m_all_classes;
    bool has_class(int class_id) const;

  public:
    void set_class(int elem, int class_id);
    bool has_elem_a_class(int elem) const;
    int get_class_of_elem(int elem) const;
    const IntHashSet& get_elems_of_class(int class_id) const;
    std::string to_string() const;
    void print() const;
    const IntHashSet& get_all_classes() const;
};

std::pair<const EquivalenceClasses, const EquivalenceClasses>
build_equivalence_classes(const Shape& shape, const UndirectedGraph& graph);

std::tuple<
    DirectedGraph,
    DirectedGraph,
    std::map<std::pair<int, int>, std::pair<int, int>>,
    std::map<std::pair<int, int>, std::pair<int, int>>>
equivalence_classes_to_ordering(
    const EquivalenceClasses& equivalence_classes_x,
    const EquivalenceClasses& equivalence_classes_y,
    const UndirectedGraph& graph,
    const Shape& shape
);

#endif