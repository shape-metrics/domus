#pragma once

#include <memory>
#include <string>

#include "domus/core/color.hpp"

enum class Attribute {
    NODES_COLOR,
    NODES_POSITION,
    // EDGES_COLOR,
    // NODES_WEIGHT,
    // EDGES_WEIGHT,
    // NODES_STRING_LABEL,
    // EDGES_STRING_LABEL,
};

std::string attribute_to_string(Attribute attribute);

class GraphAttributesImpl;

class GraphAttributes {
    std::unique_ptr<GraphAttributesImpl> m_graph_attributes;

  public:
    explicit GraphAttributes();
    GraphAttributes(const GraphAttributes&) = delete;
    GraphAttributes& operator=(const GraphAttributes&) = delete;
    GraphAttributes(GraphAttributes&&) noexcept;
    GraphAttributes& operator=(GraphAttributes&&) noexcept;
    bool has_attribute(Attribute attribute) const;
    void add_attribute(Attribute attribute);
    void remove_attribute(Attribute attribute);
    void remove_nodes_attribute(size_t node_id);
    // node color
    void set_node_color(size_t node_id, Color color);
    Color get_node_color(size_t node_id) const;
    void change_node_color(size_t node_id, Color color);
    // position
    void set_position(size_t node_id, int x, int y);
    void change_position(size_t node_id, int x, int y);
    void change_position_x(size_t node_id, int x);
    void change_position_y(size_t node_id, int y);
    int get_position_x(size_t node_id) const;
    int get_position_y(size_t node_id) const;
    bool has_position(size_t node_id) const;
    void remove_position(size_t node_id);
    ~GraphAttributes();
};