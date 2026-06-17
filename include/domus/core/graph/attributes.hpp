#pragma once

#include <optional>
#include <string>

#include "domus/core/color.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/drawing/draw_elements.hpp"
#include "domus/drawing/drawer.hpp"

namespace domus::graph {

enum class Attribute {
    NODES_COLOR,
    EDGES_COLOR,
    NODES_POSITION,
    HIDDEN_NODES,
    HIDDEN_EDGES,
    // NODES_WEIGHT,
    // EDGES_WEIGHT,
};

std::string attribute_to_string(Attribute attribute);

class Attributes {
    std::optional<utilities::NodesLabels<color::ColorRGB>> m_nodes_color = std::nullopt;
    std::optional<utilities::EdgesLabels<color::ColorRGB>> m_edges_color = std::nullopt;
    std::optional<utilities::NodesLabels<drawing::Point2D>> m_nodes_position = std::nullopt;
    std::optional<utilities::NodesContainer> m_hidden_nodes = std::nullopt;
    std::optional<utilities::EdgesContainer> m_hidden_edges = std::nullopt;

  public:
    bool has_attribute(Attribute attribute) const;
    void add_attribute(Attribute attribute);
    void remove_attribute(Attribute attribute);
    void remove_nodes_attribute(size_t node_id);
    void remove_edges_attribute(size_t edge_id);
    // node color
    void set_node_color(size_t node_id, color::ColorRGB color);
    bool has_node_color(size_t node_id) const;
    color::ColorRGB get_node_color(size_t node_id) const;
    void change_node_color(size_t node_id, color::ColorRGB color);
    // edge color
    void set_edge_color(size_t edge_id, color::ColorRGB color);
    bool has_edge_color(size_t edge_id) const;
    color::ColorRGB get_edge_color(size_t edge_id) const;
    void change_edge_color(size_t edge_id, color::ColorRGB color);
    // position
    void set_position(size_t node_id, double x, double y);
    void change_position(size_t node_id, double x, double y);
    void change_position_x(size_t node_id, double x);
    void change_position_y(size_t node_id, double y);
    double get_position_x(size_t node_id) const;
    double get_position_y(size_t node_id) const;
    const drawing::Point2D& get_position(size_t node_id) const;
    bool has_position(size_t node_id) const;
    void remove_position(size_t node_id);
    // hidden edge
    void hide_edge(size_t edge_id);
    void unhide_edge(size_t edge_id);
    bool is_edge_hidden(size_t edge_id) const;
    // hidden node
    void hide_node(size_t node_id);
    void unhide_node(size_t node_id);
    bool is_node_hidden(size_t node_id) const;

    drawing::Drawer build_drawer(const graph::Graph& graph) const;
    void visualize(const graph::Graph& graph) const;
};

} // namespace domus::graph