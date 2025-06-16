#ifndef MY_GRAPH_ATTRIBUTES_H
#define MY_GRAPH_ATTRIBUTES_H

#include <any>
#include <unordered_map>

#include "core/utils.hpp"

enum class Attribute {
  NODES_COLOR,
  NODES_POSITION,
  // EDGES_COLOR,
  // NODES_WEIGHT,
  // EDGES_WEIGHT,
  // NODES_STRING_LABEL,
  // EDGES_STRING_LABEL,
  // NODES_ANY_LABEL,
  EDGES_ANY_LABEL
};

class GraphAttributes {
 private:
  std::unordered_map<Attribute, std::unordered_map<int, std::any>>
      mattribute_to_node;
  bool has_attribute_by_id(Attribute attribute, int id) const;

 public:
  bool has_attribute(Attribute attribute) const;
  void add_attribute(Attribute attribute);
  void remove_attribute(Attribute attribute);
  // node color
  void set_node_color(int node_id, Color color);
  Color get_node_color(int node_id) const;
  void change_node_color(int node_id, Color color);
  // edge label
  void set_edge_any_label(int edge_id, const std::any& label);
  const std::any& get_edge_any_label(int edge_id) const;
  void remove_nodes_attribute(int node_id);
  // position
  void set_position(int node, int x, int y);
  void change_position(int node, int x, int y);
  void change_position_x(int node, int x);
  void change_position_y(int node, int y);
  int get_position_x(int node) const;
  int get_position_y(int node) const;
  bool has_position(int node) const;
  void remove_position(int node);
};

#endif