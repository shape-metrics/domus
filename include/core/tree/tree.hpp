#ifndef MY_TREE_H
#define MY_TREE_H

#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/utils.hpp"

class TreeNode;

class Tree {
 private:
  int m_root_id;
  int m_next_node_id = 0;
  int m_next_edge_id = 0;
  std::unordered_map<int, std::unique_ptr<TreeNode>> m_nodeid_to_node_map;
  std::unordered_map<int, std::unordered_set<const TreeNode*>>
      m_nodeid_to_childrenid;
  std::unordered_map<int, int> m_nodeid_to_parentid;
  TreeNode& add_node_with_id(int id);

 public:
  Tree(int root_id) : m_root_id(root_id) { add_node_with_id(root_id); }
  const TreeNode& get_node_by_id(int id) const;
  auto get_nodes() const {
    return m_nodeid_to_node_map |
           std::views::transform([](const auto& pair) -> const TreeNode& {
             return *pair.second;
           });
  }
  bool is_root(int id) const { return !m_nodeid_to_parentid.contains(id); }
  bool has_edge(int id1, int id2) const;
  const TreeNode& get_parent(int id) const {
    if (is_root(id))
      throw std::runtime_error("Tree::get_parent: root node has no parent");
    return get_node_by_id(m_nodeid_to_parentid.at(id));
  }
  bool has_node(int id) const { return m_nodeid_to_node_map.contains(id); }
  void add_node(int id, int parent_id) {
    if (!has_node(parent_id))
      throw std::runtime_error("Tree::add_node: parent id does not exists");
    TreeNode& node = add_node_with_id(id);
    m_nodeid_to_parentid[id] = parent_id;
    m_nodeid_to_childrenid[parent_id].insert(&node);
    m_nodeid_to_childrenid[id] = {};
  }
  void add_node(int parent_id) {
    while (has_node(m_next_node_id)) m_next_node_id++;
    add_node(m_next_node_id++, parent_id);
  }
  auto get_children(int node_id) const {
    if (!has_node(node_id))
      throw std::runtime_error("Tree::get_children: node not found");
    return m_nodeid_to_childrenid.at(node_id) |
           std::views::transform(
               [](const TreeNode* child) -> const TreeNode& { return *child; });
  }
  int size() const { return m_nodeid_to_node_map.size(); }
  const std::string to_string() const;
  void print() const { std::cout << to_string() << std::endl; }
};

class TreeNode {
 private:
  int m_id;
  const Tree& m_owner;

 public:
  TreeNode(int id, const Tree& owner) : m_id(id), m_owner(owner) {}
  int get_id() const { return m_id; }
  const TreeNode& get_parent() const { return m_owner.get_parent(m_id); }
  auto get_children() const { return m_owner.get_children(m_id); }
  const std::string to_string() const;
  bool is_root() const { return m_owner.is_root(m_id); }
  void print() const { std::cout << to_string() << std::endl; }
};

#endif