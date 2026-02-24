#include "domus/core/graph/attributes.hpp"

#include <any>
#include <cassert>
#include <initializer_list>
#include <ranges>
#include <unordered_map>

using namespace std;

class GraphAttributesImpl {
    std::unordered_map<Attribute, std::unordered_map<int, std::any>> mattribute_to_node;
    bool has_attribute_by_id(Attribute attribute, int id) const;

  public:
    bool has_attribute(Attribute attribute) const;
    void add_attribute(Attribute attribute);
    void remove_attribute(Attribute attribute);
    void remove_nodes_attribute(int node_id);
    // node color
    void set_node_color(int node_id, Color color);
    Color get_node_color(int node_id) const;
    void change_node_color(int node_id, Color color);
    // position
    void set_position(int node_id, int x, int y);
    void change_position(int node_id, int x, int y);
    void change_position_x(int node_id, int x);
    void change_position_y(int node_id, int y);
    int get_position_x(int node_id) const;
    int get_position_y(int node_id) const;
    bool has_position(int node_id) const;
    void remove_position(int node_id);
};

string attribute_to_string(Attribute attribute) {
    switch (attribute) {
    case Attribute::NODES_COLOR:
        return "NODES_COLOR";
    case Attribute::NODES_POSITION:
        return "NODES_POSITION";
    default:
        assert(false && "attribute_to_string: invalid attribute");
        return "ERROR";
    }
}

struct NodePosition {
    int x_m;
    int y_m;
    NodePosition(int x, int y) : x_m(x), y_m(y) {}
    bool operator==(const NodePosition& other) const {
        return x_m == other.x_m && y_m == other.y_m;
    }
};

bool GraphAttributesImpl::has_attribute(const Attribute attribute) const {
    return mattribute_to_node.contains(attribute);
}

void GraphAttributesImpl::add_attribute(const Attribute attribute) {
    assert(!has_attribute(attribute) && "GraphAttributes::add_attribute: already has attribute");
    mattribute_to_node[attribute] = {};
}

void GraphAttributesImpl::remove_attribute(const Attribute attribute) {
    assert(
        has_attribute(attribute) && "GraphAttributes::remove_attribute: does not have attribute"
    );
    mattribute_to_node.erase(attribute);
}

void GraphAttributesImpl::remove_nodes_attribute(const int node_id) {
    for (auto nodes_attributes : mattribute_to_node | std::views::values)
        nodes_attributes.erase(node_id);
}

bool GraphAttributesImpl::has_attribute_by_id(const Attribute attribute, const int id) const {
    assert(
        has_attribute(attribute) && "GraphAttributes::has_attribute_by_id: does not have attribute"
    );
    return mattribute_to_node.at(attribute).contains(id);
}

void GraphAttributesImpl::set_node_color(const int node_id, const Color color) {
    assert(
        !has_attribute_by_id(Attribute::NODES_COLOR, node_id) &&
        "GraphAttributes::set_node_color: the node does not have a color"
    );
    mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

Color GraphAttributesImpl::get_node_color(const int node_id) const {
    assert(
        has_attribute_by_id(Attribute::NODES_COLOR, node_id) &&
        "GraphAttributes::get_node_color: the node does not have a color"
    );
    return std::any_cast<Color>(mattribute_to_node.at(Attribute::NODES_COLOR).at(node_id));
}

void GraphAttributesImpl::change_node_color(const int node_id, const Color color) {
    assert(
        has_attribute_by_id(Attribute::NODES_COLOR, node_id) &&
        "GraphAttributes::change_node_color: the node does not have a color"
    );
    mattribute_to_node.at(Attribute::NODES_COLOR)[node_id] = color;
}

void GraphAttributesImpl::change_position(const int node_id, const int x, const int y) {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::change_position: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "GraphAttributes::change_position: node does not have a position"
    );
    auto& position =
        std::any_cast<NodePosition&>(mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.x_m = x;
    position.y_m = y;
}

void GraphAttributesImpl::change_position_x(const int node_id, const int x) {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::change_position_x: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "GraphAttributes::change_position: node does not have a position"
    );
    auto& position =
        std::any_cast<NodePosition&>(mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.x_m = x;
}

void GraphAttributesImpl::change_position_y(const int node_id, const int y) {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::change_position_y: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "GraphAttributes::change_position_y: node does not have a position"
    );
    auto& position =
        std::any_cast<NodePosition&>(mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id));
    position.y_m = y;
}

void GraphAttributesImpl::set_position(const int node_id, const int x, const int y) {
    assert(
        !has_position(node_id) && "GraphAttributes::set_position_x: node already has a position"
    );
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::set_position_x Does not have NODES_POSITION attribute"
    );
    mattribute_to_node[Attribute::NODES_POSITION][node_id] = NodePosition(x, y);
}

int GraphAttributesImpl::get_position_x(const int node_id) const {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::get_position_x: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "GraphAttributes::get_position_x: node does not have a position"
    );
    return std::any_cast<NodePosition>(mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id))
        .x_m;
}

int GraphAttributesImpl::get_position_y(const int node_id) const {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::get_position_y: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "GraphAttributes::get_position_y: node does not have a position"
    );
    return std::any_cast<NodePosition>(mattribute_to_node.at(Attribute::NODES_POSITION).at(node_id))
        .y_m;
}

bool GraphAttributesImpl::has_position(const int node_id) const {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::has_position: does not have NODES_POSITION attribute"
    );
    return mattribute_to_node.at(Attribute::NODES_POSITION).contains(node_id);
}

void GraphAttributesImpl::remove_position(const int node_id) {
    assert(
        has_attribute(Attribute::NODES_POSITION) &&
        "GraphAttributes::remove_position: does not have NODES_POSITION attribute"
    );
    assert(
        has_position(node_id) && "NodesPositions::remove_position: node does not have a position"
    );
    mattribute_to_node.at(Attribute::NODES_POSITION).erase(node_id);
}

GraphAttributes::GraphAttributes() { m_graph_attributes = std::make_unique<GraphAttributesImpl>(); }

GraphAttributes::GraphAttributes(GraphAttributes&&) noexcept = default;

GraphAttributes& GraphAttributes::operator=(GraphAttributes&&) noexcept = default;

bool GraphAttributes::has_attribute(Attribute attribute) const {
    return m_graph_attributes->has_attribute(attribute);
}

void GraphAttributes::add_attribute(Attribute attribute) {
    m_graph_attributes->add_attribute(attribute);
}

void GraphAttributes::remove_attribute(Attribute attribute) {
    m_graph_attributes->remove_attribute(attribute);
}

void GraphAttributes::remove_nodes_attribute(int node_id) {
    m_graph_attributes->remove_nodes_attribute(node_id);
}

// node color
void GraphAttributes::set_node_color(int node_id, Color color) {
    m_graph_attributes->set_node_color(node_id, color);
}

Color GraphAttributes::get_node_color(int node_id) const {
    return m_graph_attributes->get_node_color(node_id);
}

void GraphAttributes::change_node_color(int node_id, Color color) {
    m_graph_attributes->change_node_color(node_id, color);
}

// position
void GraphAttributes::set_position(int node_id, int x, int y) {
    m_graph_attributes->set_position(node_id, x, y);
}

void GraphAttributes::change_position(int node_id, int x, int y) {
    m_graph_attributes->change_position(node_id, x, y);
}

void GraphAttributes::change_position_x(int node_id, int x) {
    m_graph_attributes->change_position_x(node_id, x);
}

void GraphAttributes::change_position_y(int node_id, int y) {
    m_graph_attributes->change_position_y(node_id, y);
}

int GraphAttributes::get_position_x(int node_id) const {
    return m_graph_attributes->get_position_x(node_id);
}

int GraphAttributes::get_position_y(int node_id) const {
    return m_graph_attributes->get_position_y(node_id);
}

bool GraphAttributes::has_position(int node_id) const {
    return m_graph_attributes->has_position(node_id);
}

void GraphAttributes::remove_position(int node_id) { m_graph_attributes->remove_position(node_id); }

GraphAttributes::~GraphAttributes() = default;