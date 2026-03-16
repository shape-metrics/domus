#include "domus/orthogonal/shape/node_type.hpp"

#include <cstddef>
#include <unordered_map>

#include "../../core/domus_assert.hpp"

class NodesTypesImpl {
    std::unordered_map<size_t, NodeType> m_nodes_types;

  public:
    bool has_node_type(size_t node_id) const;
    void set_node_type(size_t node_id, NodeType type);
    void change_node_type(size_t node_id, NodeType type);
    NodeType get_node_type(size_t node_id) const;
    void remove_node_type(size_t node_id);
};

void NodesTypesImpl::set_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        !has_node_type(node_id),
        "NodesTypesImpl::set_node_type: the node already has a type"
    );
    m_nodes_types[node_id] = type;
}

NodeType NodesTypesImpl::get_node_type(size_t node_id) const {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::get_node_type: the node does not have a type"
    );
    return m_nodes_types.at(node_id);
}

void NodesTypesImpl::change_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::change_node_type: the node does not have a type"
    );
    m_nodes_types[node_id] = type;
}

bool NodesTypesImpl::has_node_type(size_t node_id) const {
    return m_nodes_types.find(node_id) != m_nodes_types.end();
}

void NodesTypesImpl::remove_node_type(size_t node_id) {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::remove_node_type: the node does not have a type"
    );
    m_nodes_types.erase(node_id);
}

NodesTypes::NodesTypes() { m_nodes_types = std::make_unique<NodesTypesImpl>(); }

NodesTypes::NodesTypes(NodesTypes&&) noexcept = default;

NodesTypes& NodesTypes::operator=(NodesTypes&&) noexcept = default;

void NodesTypes::set_node_type(size_t node_id, NodeType type) {
    m_nodes_types->set_node_type(node_id, type);
}

void NodesTypes::change_node_type(size_t node_id, NodeType type) {
    m_nodes_types->change_node_type(node_id, type);
}

NodeType NodesTypes::get_node_type(size_t node_id) const {
    return m_nodes_types->get_node_type(node_id);
}

bool NodesTypes::has_node_type(size_t node_id) const {
    return m_nodes_types->has_node_type(node_id);
}

void NodesTypes::remove_node_type(size_t node_id) { m_nodes_types->remove_node_type(node_id); }

NodesTypes::~NodesTypes() = default;