#pragma once

#include <memory>

enum class NodeType { VERTEX, BEND, MINI_BEND };

class NodesTypesImpl;

// TODO usare questa invece di usare i colori del GraphAttributes per rappresentare i corner
// TODO cambiare implementazione (non serve map e memory)
// TODO aggiungere namespace
class NodesTypes {
    std::unique_ptr<NodesTypesImpl> m_nodes_types;

  public:
    explicit NodesTypes();
    NodesTypes(const NodesTypes&) = delete;
    NodesTypes& operator=(const NodesTypes&) = delete;
    NodesTypes(NodesTypes&&) noexcept;
    NodesTypes& operator=(NodesTypes&&) noexcept;
    void set_node_type(size_t node_id, NodeType type);
    NodeType get_node_type(size_t node_id) const;
    void change_node_type(size_t node_id, NodeType type);
    void remove_node_type(size_t node_id);
    bool has_node_type(size_t node_id) const;
    ~NodesTypes();
};