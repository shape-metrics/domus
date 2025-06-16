#include "orthogonal/equivalence_classes.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>

bool EquivalenceClasses::has_class(int class_id) const {
  return m_class_to_elems.contains(class_id);
}

void EquivalenceClasses::set_class(int elem, int class_id) {
  if (has_elem_a_class(elem))
    throw std::runtime_error(
        "EquivalenceClasses::set_class elem already has an assigned class");
  m_elem_to_class[elem] = class_id;
  m_class_to_elems[class_id].insert(elem);
}

bool EquivalenceClasses::has_elem_a_class(int elem) const {
  return m_elem_to_class.contains(elem);
}

int EquivalenceClasses::get_class_of_elem(int elem) const {
  if (!has_elem_a_class(elem))
    throw std::runtime_error(
        "EquivalenceClasses::get_class elem does not have a class");
  return m_elem_to_class.at(elem);
}

const std::unordered_set<int>& EquivalenceClasses::get_elems_of_class(
    int class_id) const {
  if (!has_class(class_id))
    throw std::runtime_error(
        "EquivalenceClasses::get_elems class does not exist");
  return m_class_to_elems.at(class_id);
}

std::string EquivalenceClasses::to_string() const {
  std::string result = "EquivalenceClasses:\n";
  for (const auto& pair : m_class_to_elems) {
    result += "Class " + std::to_string(pair.first) + ": ";
    for (int elem : pair.second) result += std::to_string(elem) + " ";
    result += "\n";
  }
  return result;
}

void EquivalenceClasses::print() const {
  std::cout << to_string() << std::endl;
}

void directional_node_expander(
    const Shape& shape, const Graph& graph, const GraphNode& node, int class_id,
    EquivalenceClasses& equivalence_classes,
    std::function<bool(const Shape&, int, int)> is_direction_wrong) {
  int node_id = node.get_id();
  equivalence_classes.set_class(node_id, class_id);
  for (auto& edge : node.get_edges()) {
    int neighbor_id = edge.get_to().get_id();
    if (equivalence_classes.has_elem_a_class(neighbor_id)) continue;
    if (is_direction_wrong(shape, node_id, neighbor_id)) continue;
    directional_node_expander(shape, graph, edge.get_to(), class_id,
                              equivalence_classes, is_direction_wrong);
  }
}

void horizontal_node_expander(const Shape& shape, const Graph& graph,
                              const GraphNode& node, int class_id,
                              GraphEdgeHashSet& is_edge_visited,
                              EquivalenceClasses& equivalence_classes) {
  auto is_direction_wrong = [](const Shape& shape, int i, int j) {
    return shape.is_vertical(i, j);
  };
  directional_node_expander(shape, graph, node, class_id, equivalence_classes,
                            is_direction_wrong);
}

void vertical_node_expander(const Shape& shape, const Graph& graph,
                            const GraphNode& node, int class_id,
                            GraphEdgeHashSet& is_edge_visited,
                            EquivalenceClasses& equivalence_classes) {
  auto is_direction_wrong = [](const Shape& shape, int i, int j) {
    return shape.is_horizontal(i, j);
  };
  directional_node_expander(shape, graph, node, class_id, equivalence_classes,
                            is_direction_wrong);
}

const std::pair<EquivalenceClasses, EquivalenceClasses>
build_equivalence_classes(const Shape& shape, const Graph& graph) {
  EquivalenceClasses equivalence_classes_x;
  EquivalenceClasses equivalence_classes_y;
  int next_class_x = 0;
  int next_class_y = 0;
  GraphEdgeHashSet is_edge_visited;
  for (auto& node : graph.get_nodes()) {
    if (!equivalence_classes_y.has_elem_a_class(node.get_id()))
      horizontal_node_expander(shape, graph, node, next_class_y++,
                               is_edge_visited, equivalence_classes_y);
    if (!equivalence_classes_x.has_elem_a_class(node.get_id()))
      vertical_node_expander(shape, graph, node, next_class_x++,
                             is_edge_visited, equivalence_classes_x);
  }
  for (const auto& node : graph.get_nodes()) {
    if (!equivalence_classes_x.has_elem_a_class(node.get_id()))
      equivalence_classes_x.set_class(node.get_id(), next_class_x++);
    if (!equivalence_classes_y.has_elem_a_class(node.get_id()))
      equivalence_classes_y.set_class(node.get_id(), next_class_y++);
  }
  return std::make_pair(std::move(equivalence_classes_x),
                        std::move(equivalence_classes_y));
}

std::tuple<std::unique_ptr<Graph>, std::unique_ptr<Graph>, GraphAttributes,
           GraphAttributes>
equivalence_classes_to_ordering(const EquivalenceClasses& equivalence_classes_x,
                                const EquivalenceClasses& equivalence_classes_y,
                                const Graph& graph, const Shape& shape) {
  auto ordering_x = std::make_unique<Graph>();
  auto ordering_y = std::make_unique<Graph>();
  for (int class_id : equivalence_classes_x.get_all_classes())
    ordering_x->add_node(class_id);
  for (int class_id : equivalence_classes_y.get_all_classes())
    ordering_y->add_node(class_id);
  GraphAttributes ordering_x_edge_to_graph_edge;
  GraphAttributes ordering_y_edge_to_graph_edge;
  ordering_x_edge_to_graph_edge.add_attribute(Attribute::EDGES_ANY_LABEL);
  ordering_y_edge_to_graph_edge.add_attribute(Attribute::EDGES_ANY_LABEL);
  for (auto& node : graph.get_nodes()) {
    int i = node.get_id();
    for (auto& edge : node.get_edges()) {
      int j = edge.get_to().get_id();
      if (shape.is_right(i, j)) {
        int node_class_x = equivalence_classes_x.get_class_of_elem(i);
        int neighbor_class_x = equivalence_classes_x.get_class_of_elem(j);
        if (ordering_x->has_edge(node_class_x, neighbor_class_x)) continue;
        // if (node_class_x == neighbor_class_x) {
        //   continue;
        // }
        auto& e = ordering_x->add_edge(node_class_x, neighbor_class_x);
        ordering_x_edge_to_graph_edge.set_edge_any_label(e.get_id(),
                                                         std::make_pair(i, j));
      } else if (shape.is_up(i, j)) {
        int node_class_y = equivalence_classes_y.get_class_of_elem(i);
        int neighbor_class_y = equivalence_classes_y.get_class_of_elem(j);
        if (ordering_y->has_edge(node_class_y, neighbor_class_y)) continue;
        // if (node_class_y == neighbor_class_y) {
        //   continue;
        // }
        auto& e = ordering_y->add_edge(node_class_y, neighbor_class_y);
        ordering_y_edge_to_graph_edge.set_edge_any_label(e.get_id(),
                                                         std::make_pair(i, j));
      }
    }
  }
  return std::make_tuple(std::move(ordering_x), std::move(ordering_y),
                         std::move(ordering_x_edge_to_graph_edge),
                         std::move(ordering_y_edge_to_graph_edge));
}