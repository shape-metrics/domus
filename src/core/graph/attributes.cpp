#include "domus/core/graph/attributes.hpp"

#include <initializer_list>
#include <iostream>
#include <ranges>
#include <stdexcept>

std::string attribute_to_string(Attribute attribute) {
    switch (attribute) {
        case Attribute::NODES_COLOR:
            return "NODES_COLOR";
        case Attribute::NODES_POSITION:
            return "NODES_POSITION";
        default:
            return "UNKNOWN";
    }
}

struct NodePosition {
    int x_m;
    int y_m;
    NodePosition(const int x, const int y) {
        this->x_m = x;
        this->y_m = y;
    }
    bool operator==(const NodePosition& other) const {
        return x_m == other.x_m && y_m == other.y_m;
    }
};

bool GraphAttributes::has_attribute(const Attribute attribute) const {
    return mattribute_to_node.contains(attribute);
}

void GraphAttributes::add_attribute(const Attribute attribute) {
    if (has_attribute(attribute)) {
        std::string error =
            "GraphAttributes::add_attribute: already has this "
            "attribute " +
            attribute_to_string(attribute);
        throw std::runtime_error(error);
    }
    mattribute_to_node[attribute] = {};
}

void GraphAttributes::remove_attribute(const Attribute attribute) {
    if (!has_attribute(attribute)) {
        std::string error =
            "GraphAttributes::remove_attribute: does not have this "
            "attribute " +
            attribute_to_string(attribute);
        throw std::runtime_error(error);
    }
    mattribute_to_node.erase(attribute);
}

void GraphAttributes::remove_nodes_attribute(const int node_id) {
    for (auto nodes_attributes : mattribute_to_node | std::views::values) {
        nodes_attributes.erase(node_id);
    }
}

bool GraphAttributes::has_attribute_by_id(
    const Attribute attribute,
    const int id) const {
    if (!has_attribute(attribute))
        throw std::runtime_error(
            "GraphAttributes::has_attribute_by_id: does not have attribute");
    return mattribute_to_node.at(attribute).contains(id);
}

void GraphAttributes::set_node_color(const int node_id, const Color color) {
    if (has_attribute_by_id(Attribute::NODES_COLOR, node_id))
        throw std::runtime_error(
            "GraphAttributes::set_node_color: the node already has color");
    mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

Color GraphAttributes::get_node_color(const int node_id) const {
    if (!has_attribute_by_id(Attribute::NODES_COLOR, node_id))
        throw std::runtime_error(
            "GraphAttributes::get_node_color: the node does not have a color");
    return std::any_cast<Color>(
        mattribute_to_node.at(Attribute::NODES_COLOR).at(node_id));
}

void GraphAttributes::change_node_color(const int node_id, const Color color) {
    if (!has_attribute_by_id(Attribute::NODES_COLOR, node_id))
        throw std::runtime_error(
            "GraphAttributes::change_node_color: the node does not have a "
            "color");
    mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

void GraphAttributes::change_position(
    const int node_id,
    const int x,
    const int y) {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::change_position Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id))
        throw std::runtime_error(
            "GraphAttributes::change_position Node does not have a position");
    auto& position = std::any_cast<NodePosition&>(
        mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.x_m = x;
    position.y_m = y;
}

void GraphAttributes::change_position_x(const int node_id, const int x) {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::change_position_x Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id))
        throw std::runtime_error(
            "GraphAttributes::change_position Node does not have a position");
    auto& position = std::any_cast<NodePosition&>(
        mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.x_m = x;
}

void GraphAttributes::change_position_y(const int node_id, const int y) {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::change_position_y Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id))
        throw std::runtime_error(
            "GraphAttributes::change_position_y Node does not have a position");
    auto& position = std::any_cast<NodePosition&>(
        mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.y_m = y;
}

void GraphAttributes::set_position(
    const int node_id,
    const int x,
    const int y) {
    if (has_position(node_id))
        throw std::runtime_error(
            "GraphAttributes::set_position_x Node already has a position");
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::set_position_x Does not have NODES_POSITION "
            "attribute");
    mattribute_to_node[Attribute::NODES_POSITION][node_id] = NodePosition(x, y);
}

int GraphAttributes::get_position_x(const int node_id) const {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::get_position_x Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id)) {
        std::cout << node_id << std::endl;
        throw std::runtime_error(
            "GraphAttributes::get_position_x Node does not have a position");
    }
    return std::any_cast<NodePosition>(
               mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id))
        .x_m;
}

int GraphAttributes::get_position_y(const int node_id) const {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::get_position_y Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id))
        throw std::runtime_error(
            "GraphAttributes::get_position_y Node does not have a position");
    return std::any_cast<NodePosition>(
               mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id))
        .y_m;
}

bool GraphAttributes::has_position(const int node_id) const {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::has_position Does not have NODES_POSITION "
            "attribute");
    return mattribute_to_node.at(Attribute::NODES_POSITION).contains(node_id);
}

void GraphAttributes::remove_position(const int node_id) {
    if (!has_attribute(Attribute::NODES_POSITION))
        throw std::runtime_error(
            "GraphAttributes::remove_position Does not have NODES_POSITION "
            "attribute");
    if (!has_position(node_id))
        throw std::runtime_error(
            "NodesPositions::remove_position Node does not have a position");
    mattribute_to_node.at(Attribute::NODES_POSITION).erase(node_id);
}