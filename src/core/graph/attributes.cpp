#include "core/graph/attributes.hpp"

struct NodePosition {
  float x;
  float y;
  NodePosition(float x, float y) {
    this->x = x;
    this->y = y;
  }
  bool operator==(const NodePosition& other) const {
    return x == other.x && y == other.y;
  }
};

bool GraphAttributes::has_attribute(Attribute attribute) const {
  return mattribute_to_node.contains(attribute);
}

void GraphAttributes::add_attribute(Attribute attribute) {
  if (has_attribute(attribute))
    throw std::runtime_error(
        "GraphAttributes::add_attribute: already has this attribute");
  mattribute_to_node[attribute] = {};
}

void GraphAttributes::remove_attribute(Attribute attribute) {
  if (!has_attribute(attribute))
    throw std::runtime_error(
        "GraphAttributes::remove_attribute: does not have this attribute");
  mattribute_to_node.erase(attribute);
}

void GraphAttributes::remove_nodes_attribute(int node_id) {
  for (auto [attribute, nodes_attributes] : mattribute_to_node) {
    nodes_attributes.erase(node_id);
  }
}

bool GraphAttributes::has_attribute_by_id(Attribute attribute, int id) const {
  if (!has_attribute(attribute))
    throw std::runtime_error(
        "GraphAttributes::has_attribute_by_id: does not have attribute");
  return mattribute_to_node.at(attribute).contains(id);
}

void GraphAttributes::set_node_color(int node_id, Color color) {
  if (has_attribute_by_id(Attribute::NODES_COLOR, node_id))
    throw std::runtime_error(
        "GraphAttributes::set_node_color: the node already has color");
  mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

Color GraphAttributes::get_node_color(int node_id) const {
  if (!has_attribute_by_id(Attribute::NODES_COLOR, node_id))
    throw std::runtime_error(
        "GraphAttributes::get_node_color: the node does not have a color");
  return std::any_cast<Color>(
      mattribute_to_node.at(Attribute::NODES_COLOR).at(node_id));
}

void GraphAttributes::change_node_color(int node_id, Color color) {
  if (!has_attribute_by_id(Attribute::NODES_COLOR, node_id))
    throw std::runtime_error(
        "GraphAttributes::change_node_color: the node does not have a color");
  mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

void GraphAttributes::set_edge_any_label(int edge_id, const std::any& label) {
  if (has_attribute_by_id(Attribute::EDGES_ANY_LABEL, edge_id))
    throw std::runtime_error(
        "GraphAttributes::set_edge_any_label: the edge already has a label");
  mattribute_to_node.at(Attribute::EDGES_ANY_LABEL)[edge_id] = label;
}

const std::any& GraphAttributes::get_edge_any_label(int edge_id) const {
  if (!has_attribute_by_id(Attribute::EDGES_ANY_LABEL, edge_id))
    throw std::runtime_error(
        "GraphAttributes::get_edge_any_label: the edge does not have a label");
  return mattribute_to_node.at(Attribute::EDGES_ANY_LABEL).at(edge_id);
}

void GraphAttributes::change_position(int node, int x, int y) {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::change_position Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node))
    throw std::runtime_error(
        "GraphAttributes::change_position Node does not have a position");
  auto& position = std::any_cast<NodePosition&>(
      mattribute_to_node.at(Attribute::NODES_POSITION).at(node));
  position.x = x;
  position.y = y;
}

void GraphAttributes::change_position_x(int node, int x) {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::change_position_x Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node))
    throw std::runtime_error(
        "GraphAttributes::change_position Node does not have a position");
  auto& position = std::any_cast<NodePosition&>(
      mattribute_to_node.at(Attribute::NODES_POSITION).at(node));
  position.x = x;
}

void GraphAttributes::change_position_y(int node, int y) {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::change_position_y Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node))
    throw std::runtime_error(
        "GraphAttributes::change_position_y Node does not have a position");
  auto& position = std::any_cast<NodePosition&>(
      mattribute_to_node.at(Attribute::NODES_POSITION).at(node));
  position.y = y;
}

void GraphAttributes::set_position(int node, int x, int y) {
  if (has_position(node))
    throw std::runtime_error(
        "GraphAttributes::set_position_x Node already has a position");
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::set_position_x Does not have NODES_POSITION "
        "attribute");
  mattribute_to_node[Attribute::NODES_POSITION][node] = NodePosition(x, y);
}

int GraphAttributes::get_position_x(int node) const {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::get_position_x Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node)) {
    std::cout << node << std::endl;
    throw std::runtime_error(
        "GraphAttributes::get_position_x Node does not have a position");
  }
  return std::any_cast<NodePosition>(
             mattribute_to_node.at(Attribute::NODES_POSITION).at(node))
      .x;
}

int GraphAttributes::get_position_y(int node) const {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::get_position_y Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node))
    throw std::runtime_error(
        "GraphAttributes::get_position_y Node does not have a position");
  return std::any_cast<NodePosition>(
             mattribute_to_node.at(Attribute::NODES_POSITION).at(node))
      .y;
}

bool GraphAttributes::has_position(int node) const {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::has_position Does not have NODES_POSITION "
        "attribute");
  return mattribute_to_node.at(Attribute::NODES_POSITION).contains(node);
}

void GraphAttributes::remove_position(int node) {
  if (!has_attribute(Attribute::NODES_POSITION))
    throw std::runtime_error(
        "GraphAttributes::remove_position Does not have NODES_POSITION "
        "attribute");
  if (!has_position(node))
    throw std::runtime_error(
        "NodesPositions::remove_position Node does not have a position");
  mattribute_to_node.at(Attribute::NODES_POSITION).erase(node);
}