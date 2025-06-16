#include "core/tree/tree.hpp"

TreeNode& Tree::add_node_with_id(int id) {
  if (id < 0)
    throw std::runtime_error("Tree::add_node: id must be non-negative");
  if (has_node(id))
    throw std::runtime_error(
        "Tree::add_node: node with this id already exists");
  m_nodeid_to_node_map[id] = std::make_unique<TreeNode>(id, *this);
  m_nodeid_to_childrenid[id] = {};
  return *m_nodeid_to_node_map[id];
}

const TreeNode& Tree::get_node_by_id(int id) const {
  if (!has_node(id))
    throw std::runtime_error("Tree::get_node_by_id: node not found");
  return *m_nodeid_to_node_map.at(id);
}

const std::string TreeNode::to_string() const {
  std::string result = "Node " + std::to_string(m_id) + ": ";
  if (is_root())
    result += "root";
  else
    result += "parent: " + std::to_string(get_parent().get_id());
  result += ", children: ";
  for (const auto& child : get_children())
    result += std::to_string(child.get_id()) + " ";
  return result;
}

const std::string Tree::to_string() const {
  std::string result = "Tree:\n";
  for (const auto& node : get_nodes()) result += node.to_string() + "\n";
  return result;
}

bool Tree::has_edge(int node_id_1, int node_id_2) const {
  const auto* node_1 = &get_node_by_id(node_id_1);
  const auto* node_2 = &get_node_by_id(node_id_2);
  if (node_1->is_root()) return node_2->get_parent().get_id() == node_id_1;
  if (node_2->is_root()) return node_1->get_parent().get_id() == node_id_2;
  return &node_1->get_parent() == node_2 || &node_2->get_parent() == node_1;
}