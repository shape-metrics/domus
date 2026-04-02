#include "domus/orthogonal/shape/node_type.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::orthogonal::shape {

void NodesTypes::set_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        !has_node_type(node_id),
        "NodesTypesImpl::set_node_type: the node already has a type"
    );
    m_nodes_types.add_label(node_id, type);
}

NodeType NodesTypes::get_node_type(size_t node_id) const {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::get_node_type: the node does not have a type"
    );
    return m_nodes_types.get_label(node_id);
}

void NodesTypes::change_node_type(size_t node_id, NodeType type) {
    DOMUS_ASSERT(
        has_node_type(node_id),
        "NodesTypesImpl::change_node_type: the node does not have a type"
    );
    m_nodes_types.update_label(node_id, type);
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