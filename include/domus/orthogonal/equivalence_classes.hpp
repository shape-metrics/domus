#ifndef MY_EUIVALENCE_CLASSES_H
#define MY_EUIVALENCE_CLASSES_H

#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "domus/core/graph/graph.hpp"

class Shape;

class EquivalenceClasses {
    std::unordered_map<int, int> m_elem_to_class;
    std::unordered_map<int, std::unordered_set<int>> m_class_to_elems;
    bool has_class(int class_id) const;

  public:
    void set_class(int elem, int class_id);
    bool has_elem_a_class(int elem) const;
    int get_class_of_elem(int elem) const;
    const std::unordered_set<int>& get_elems_of_class(int class_id) const;
    std::string to_string() const;
    void print() const;
    auto get_all_classes() const {
        return m_class_to_elems |
               std::views::transform([](const auto& pair) -> int { return pair.first; });
    }
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