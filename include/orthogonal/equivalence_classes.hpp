#ifndef MY_EUIVALENCE_CLASSES_H
#define MY_EUIVALENCE_CLASSES_H

#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/graph/attributes.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/shape/shape.hpp"

class EquivalenceClasses {
 private:
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
           std::views::transform(
               [](const auto& pair) -> int { return pair.first; });
  }
};

const std::pair<EquivalenceClasses, EquivalenceClasses>
build_equivalence_classes(const Shape& shape, const Graph& graph);

std::tuple<std::unique_ptr<Graph>, std::unique_ptr<Graph>, GraphAttributes,
           GraphAttributes>
equivalence_classes_to_ordering(const EquivalenceClasses& equivalence_classes_x,
                                const EquivalenceClasses& equivalence_classes_y,
                                const Graph& graph, const Shape& shape);

#endif