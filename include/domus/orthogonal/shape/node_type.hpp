#pragma once

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::orthogonal::shape {

enum class NodeType { VERTEX, BEND, MINI_BEND, INVALID };

// TODO usare questa invece di usare i colori del GraphAttributes per rappresentare i corner
class NodesTypes {
    graph::utilities::NodesLabels<NodeType> m_nodes_types;

  public:
    NodesTypes(const graph::Graph& graph);
    void set_node_type(size_t node_id, NodeType type);
    NodeType get_node_type(size_t node_id) const;
    void change_node_type(size_t node_id, NodeType type);
    void remove_node_type(size_t node_id);
    bool has_node_type(size_t node_id) const;
};

} // namespace domus::orthogonal::shape