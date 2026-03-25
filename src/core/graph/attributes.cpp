#include "domus/core/graph/attributes.hpp"

#include "../domus_debug.hpp"

namespace domus::graph {

std::string attribute_to_string(Attribute attribute) {
    switch (attribute) {
    case Attribute::NODES_COLOR:
        return "NODES_COLOR";
    case Attribute::NODES_POSITION:
        return "NODES_POSITION";
    default:
        DOMUS_ASSERT(false, "attribute_to_string: invalid attribute");
        return "ERROR";
    }
}

bool GraphAttributes::has_attribute(Attribute attribute) const {
    switch (attribute) {
    case Attribute::NODES_COLOR:
        return m_nodes_color.has_value();
    case Attribute::NODES_POSITION:
        return m_nodes_position.has_value();
    default:
        DOMUS_ASSERT(false, "GraphAttributes::has_attribute: invalid attribute");
        return false;
    }
}

void GraphAttributes::add_attribute(Attribute attribute) {
    DOMUS_ASSERT(
        !has_attribute(attribute),
        "GraphAttributes::add_attribute: attribute already exists"
    );
    switch (attribute) {
    case Attribute::NODES_COLOR:
        m_nodes_color = std::vector<std::optional<Color>>();
        break;
    case Attribute::NODES_POSITION:
        m_nodes_position = std::vector<std::optional<NodePosition>>();
        break;
    default:
        DOMUS_ASSERT(false, "GraphAttributes::add_attribute: invalid attribute");
        break;
    }
}

void GraphAttributes::remove_attribute(Attribute attribute) {
    DOMUS_ASSERT(
        has_attribute(attribute),
        "GraphAttributes::remove_attribute: attribute does not exist"
    );
    switch (attribute) {
    case Attribute::NODES_COLOR:
        m_nodes_color.reset();
        break;
    case Attribute::NODES_POSITION:
        m_nodes_position.reset();
        break;
    default:
        DOMUS_ASSERT(false, "GraphAttributes::remove_attribute: invalid attribute");
        break;
    }
}

void GraphAttributes::remove_nodes_attribute(size_t node_id) {
    if (has_attribute(Attribute::NODES_COLOR))
        m_nodes_color->at(node_id) = std::nullopt;
    if (has_attribute(Attribute::NODES_POSITION))
        m_nodes_position->at(node_id) = std::nullopt;
}

// node color
void GraphAttributes::set_node_color(size_t node_id, Color color) {
    while (m_nodes_color->size() <= node_id)
        m_nodes_color->push_back(std::nullopt);
    DOMUS_ASSERT(
        !m_nodes_color->at(node_id).has_value(),
        "GraphAttributes::set_node_color: node already has a color"
    );
    m_nodes_color->at(node_id) = color;
}

Color GraphAttributes::get_node_color(size_t node_id) const {
    DOMUS_ASSERT(
        m_nodes_color->at(node_id).has_value(),
        "GraphAttributes::get_node_color: node does not have a color"
    );
    return m_nodes_color->at(node_id).value();
}

void GraphAttributes::change_node_color(size_t node_id, Color color) {
    DOMUS_ASSERT(
        m_nodes_color->at(node_id).has_value(),
        "GraphAttributes::change_node_color: node does not have a color"
    );
    m_nodes_color->at(node_id) = color;
}

// position
void GraphAttributes::set_position(size_t node_id, int x, int y) {
    while (m_nodes_position->size() <= node_id)
        m_nodes_position->push_back(std::nullopt);
    DOMUS_ASSERT(
        !m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::set_position: node already has a position"
    );
    m_nodes_position->at(node_id) = NodePosition(x, y);
}

void GraphAttributes::change_position(size_t node_id, int x, int y) {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::change_position: node does not have a position"
    );
    m_nodes_position->at(node_id) = NodePosition(x, y);
}

void GraphAttributes::change_position_x(size_t node_id, int x) {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::change_position_x: node does not have a position"
    );
    m_nodes_position->at(node_id)->x_m = x;
}

void GraphAttributes::change_position_y(size_t node_id, int y) {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::change_position_y: node does not have a position"
    );
    m_nodes_position->at(node_id)->y_m = y;
}

int GraphAttributes::get_position_x(size_t node_id) const {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::get_position_x: node does not have a position"
    );
    return m_nodes_position->at(node_id)->x_m;
}

int GraphAttributes::get_position_y(size_t node_id) const {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::get_position_y: node does not have a position"
    );
    return m_nodes_position->at(node_id)->y_m;
}

bool GraphAttributes::has_position(size_t node_id) const {
    return m_nodes_position->at(node_id).has_value();
}

void GraphAttributes::remove_position(size_t node_id) {
    DOMUS_ASSERT(
        m_nodes_position->at(node_id).has_value(),
        "GraphAttributes::remove_position: node does not have a position"
    );
    m_nodes_position->at(node_id) = std::nullopt;
}

} // namespace domus::graph