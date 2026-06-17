#include "domus/core/graph/attributes.hpp"

#include <limits>

#include "domus/core/color.hpp"
#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/drawing/drawer.hpp"
#include "domus/drawing/linear_scale.hpp"

namespace domus::graph {
using color::ColorRGB;
using namespace drawing;

std::string attribute_to_string(Attribute attribute) {
    switch (attribute) {
    case Attribute::NODES_COLOR:
        return "NODES_COLOR";
    case Attribute::EDGES_COLOR:
        return "EDGES_COLOR";
    case Attribute::NODES_POSITION:
        return "NODES_POSITION";
    case Attribute::HIDDEN_NODES:
        return "HIDDEN_NODES";
    case Attribute::HIDDEN_EDGES:
        return "HIDDEN_EDGES";
    default:
        DOMUS_ASSERT(false, "attribute_to_string: invalid attribute");
        return "ERROR";
    }
}

bool Attributes::has_attribute(Attribute attribute) const {
    switch (attribute) {
    case Attribute::NODES_COLOR:
        return m_nodes_color.has_value();
    case Attribute::NODES_POSITION:
        return m_nodes_position.has_value();
    case Attribute::EDGES_COLOR:
        return m_edges_color.has_value();
    case Attribute::HIDDEN_EDGES:
        return m_hidden_edges.has_value();
    case Attribute::HIDDEN_NODES:
        return m_hidden_nodes.has_value();
    default:
        DOMUS_ASSERT(false, "GraphAttributes::has_attribute: invalid attribute");
        return false;
    }
}

void Attributes::add_attribute(Attribute attribute) {
    DOMUS_ASSERT(
        !has_attribute(attribute),
        "GraphAttributes::add_attribute: attribute already exists"
    );
    switch (attribute) {
    case Attribute::NODES_COLOR:
        m_nodes_color = utilities::NodesLabels<ColorRGB>();
        break;
    case Attribute::NODES_POSITION:
        m_nodes_position = utilities::NodesLabels<drawing::Point2D>();
        break;
    case Attribute::EDGES_COLOR:
        m_edges_color = utilities::EdgesLabels<ColorRGB>();
        break;
    case Attribute::HIDDEN_NODES:
        m_hidden_nodes = utilities::NodesContainer();
        break;
    case Attribute::HIDDEN_EDGES:
        m_hidden_edges = utilities::EdgesContainer();
        break;
    default:
        DOMUS_ASSERT(false, "GraphAttributes::add_attribute: invalid attribute");
        break;
    }
}

void Attributes::remove_attribute(Attribute attribute) {
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
    case Attribute::EDGES_COLOR:
        m_edges_color.reset();
        break;
    case Attribute::HIDDEN_NODES:
        m_hidden_nodes.reset();
        break;
    case Attribute::HIDDEN_EDGES:
        m_hidden_edges.reset();
        break;
    default:
        DOMUS_ASSERT(false, "GraphAttributes::remove_attribute: invalid attribute");
        break;
    }
}

void Attributes::remove_nodes_attribute(size_t node_id) {
    if (has_attribute(Attribute::NODES_COLOR) && m_nodes_color->has_label(node_id))
        m_nodes_color->erase_label(node_id);
    if (has_attribute(Attribute::NODES_POSITION) && m_nodes_position->has_label(node_id))
        m_nodes_position->erase_label(node_id);
    if (has_attribute(Attribute::HIDDEN_NODES) && m_hidden_nodes->has_node(node_id))
        m_hidden_nodes->erase(node_id);
}

void Attributes::remove_edges_attribute(size_t edge_id) {
    if (has_attribute(Attribute::EDGES_COLOR) && m_edges_color->has_label(edge_id))
        m_edges_color->erase_label(edge_id);
    if (has_attribute(Attribute::HIDDEN_EDGES) && m_hidden_edges->has_edge(edge_id))
        m_hidden_edges->erase(edge_id);
}

// node color

void Attributes::set_node_color(size_t node_id, ColorRGB color) {
    m_nodes_color->add_label(node_id, color);
}

bool Attributes::has_node_color(size_t node_id) const { return m_nodes_color->has_label(node_id); }

ColorRGB Attributes::get_node_color(size_t node_id) const {
    return m_nodes_color->get_label(node_id);
}

void Attributes::change_node_color(size_t node_id, ColorRGB color) {
    m_nodes_color->update_label(node_id, color);
}

// edge color

void Attributes::set_edge_color(size_t edge_id, ColorRGB color) {
    m_edges_color->add_label(edge_id, color);
}

bool Attributes::has_edge_color(size_t edge_id) const { return m_edges_color->has_label(edge_id); }

ColorRGB Attributes::get_edge_color(size_t edge_id) const {
    return m_edges_color->get_label(edge_id);
}

void Attributes::change_edge_color(size_t edge_id, ColorRGB color) {
    m_edges_color->update_label(edge_id, color);
}

// node position

void Attributes::set_position(size_t node_id, double x, double y) {
    m_nodes_position->add_label(node_id, drawing::Point2D(x, y));
}

void Attributes::change_position(size_t node_id, double x, double y) {
    m_nodes_position->update_label(node_id, drawing::Point2D(x, y));
}

void Attributes::change_position_x(size_t node_id, double x) {
    m_nodes_position->get_label(node_id).x = x;
}

void Attributes::change_position_y(size_t node_id, double y) {
    m_nodes_position->get_label(node_id).y = y;
}

double Attributes::get_position_x(size_t node_id) const {
    return m_nodes_position->get_label(node_id).x;
}

double Attributes::get_position_y(size_t node_id) const {
    return m_nodes_position->get_label(node_id).y;
}

const drawing::Point2D& Attributes::get_position(size_t node_id) const {
    return m_nodes_position->get_label(node_id);
}

bool Attributes::has_position(size_t node_id) const { return m_nodes_position->has_label(node_id); }

void Attributes::remove_position(size_t node_id) { m_nodes_position->erase_label(node_id); }

// hidden edge

void Attributes::hide_edge(size_t edge_id) { m_hidden_edges->add_edge(edge_id); }

void Attributes::unhide_edge(size_t edge_id) { m_hidden_edges->erase(edge_id); }

bool Attributes::is_edge_hidden(size_t edge_id) const { return m_hidden_edges->has_edge(edge_id); }

// hidden node
void Attributes::hide_node(size_t node_id) { m_hidden_nodes->add_node(node_id); }

void Attributes::unhide_node(size_t node_id) { m_hidden_nodes->erase(node_id); }

bool Attributes::is_node_hidden(size_t node_id) const { return m_hidden_nodes->has_node(node_id); }

Drawer Attributes::build_drawer(const Graph& graph) const {
    double max_x = -std::numeric_limits<double>().max();
    double max_y = -std::numeric_limits<double>().max();
    for (const size_t node_id : graph.get_nodes_ids()) {
        max_x = std::max(max_x, get_position_x(node_id));
        max_y = std::max(max_y, get_position_y(node_id));
    }
    double min_x = std::numeric_limits<double>().max();
    double min_y = std::numeric_limits<double>().max();
    for (const size_t node_id : graph.get_nodes_ids()) {
        min_x = std::min(min_x, get_position_x(node_id));
        min_y = std::min(min_y, get_position_y(node_id));
    }
    const double width = max_x - min_x;
    const double height = max_y - min_y;
    Drawer drawer{width, height};
    auto scale_x = ScaleLinear(min_x, max_x, 0, width);
    auto scale_y = ScaleLinear(min_y, max_y, 0, height);
    std::vector<Point2D> points;
    points.resize(graph.get_number_of_nodes());
    for (const size_t node_id : graph.get_nodes_ids()) {
        const double x = scale_x.map(get_position_x(node_id));
        const double y = scale_y.map(get_position_y(node_id));
        while (points.size() <= node_id)
            points.emplace_back();
        points[node_id] = Point2D(x, y);
    }
    for (const EdgeId edge : graph.get_all_edges()) {
        if (has_attribute(Attribute::HIDDEN_EDGES) && is_edge_hidden(edge.id))
            continue;
        if (has_attribute(Attribute::EDGES_COLOR) && has_edge_color(edge.id))
            drawer.add(
                Line2D{
                    points.at(edge.edge.from_id),
                    points.at(edge.edge.to_id),
                    get_edge_color(edge.id)
                }
            );
        else
            drawer.add(Line2D{points.at(edge.edge.from_id), points.at(edge.edge.to_id), BLACK_RGB});
    }
    for (const size_t node_id : graph.get_nodes_ids()) {
        if (has_attribute(Attribute::HIDDEN_NODES) && is_node_hidden(node_id))
            continue;
        if (has_attribute(Attribute::NODES_COLOR) && has_node_color(node_id))
            drawer.add(RoundSquare2D{points.at(node_id), 20, 4, get_node_color(node_id)});
        else
            drawer.add(RoundSquare2D{points.at(node_id), 20, 4, CORNERFLOWERBLUE_RGB});
        drawer.add(Text2D{std::to_string(node_id), points.at(node_id)});
    }
    return drawer;
}

} // namespace domus::graph
