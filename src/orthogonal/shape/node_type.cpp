#include "domus/orthogonal/shape/node_type.hpp"

#include "../../core/domus_debug.hpp"

namespace domus::orthogonal::shape {

size_t node_type_to_sizet(NodeType type) {
    switch (type) {
    case NodeType::VERTEX:
        return 1;
    case NodeType::BEND:
        return 2;
    case NodeType::MINI_BEND:
        return 3;
    default:
        DOMUS_ASSERT(false, "node_type_to_sizet: invalid node type");
        return 0;
    }
}

NodeType node_type_from_sizet(size_t type) {
    switch (type) {
    case 1:
        return NodeType::VERTEX;
    case 2:
        return NodeType::BEND;
    case 3:
        return NodeType::MINI_BEND;
    }
    DOMUS_ASSERT(false, "node_type_from_sizet: invalid node type");
    return NodeType::INVALID;
}

void NodesTypes::set_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        !has_node_type(node_id),
        "NodesTypesImpl::set_node_type: the node already has a type"
    );
    m_nodes_types.add_label(node_id, node_type_to_sizet(type));
}

NodeType NodesTypes::get_node_type(size_t node_id) const {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::get_node_type: the node does not have a type"
    );
    return node_type_from_sizet(m_nodes_types.get_label(node_id));
}

void NodesTypes::change_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::change_node_type: the node does not have a type"
    );
    m_nodes_types.update_label(node_id, node_type_to_sizet(type));
}

bool NodesTypes::has_node_type(size_t node_id) const { return m_nodes_types.has_label(node_id); }

void NodesTypes::remove_node_type(size_t node_id) {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::remove_node_type: the node does not have a type"
    );
    m_nodes_types.erase_label(node_id);
}

} // namespace domus::orthogonal::shape