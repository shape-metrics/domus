#ifndef MY_GRAPH_H
#define MY_GRAPH_H

#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/utils.hpp"

class GraphNode;
class Graph;

class GraphEdge {
 private:
  const int m_id;
  const GraphNode& m_from;
  const GraphNode& m_to;
  const Graph& m_graph_owner;

 public:
  GraphEdge(int id, const GraphNode& from, const GraphNode& to,
            const Graph& owner);
  const GraphNode& get_to() const { return m_to; }
  const GraphNode& get_from() const { return m_from; }
  int get_id() const { return m_id; }
  const std::string to_string() const;
  const Graph& get_owner() const { return m_graph_owner; }
  void print() const { std::cout << to_string() << std::endl; }
};

class Graph {
 private:
  int m_next_node_id = 0;
  int m_next_edge_id = 0;
  std::unordered_map<int, std::unique_ptr<GraphNode>> m_nodeid_to_node_map;
  std::unordered_map<int, std::unordered_set<int>> m_nodeid_to_edgesid;
  std::unordered_map<int, std::unordered_map<int, int>>
      m_nodeid_to_neighborsid_to_edgesid;
  std::unordered_map<int, std::unique_ptr<GraphEdge>> m_edgeid_to_edge_map;
  std::unordered_map<int, std::unordered_set<int>>
      m_nodeid_to_incoming_edgesid_map;

 public:
  Graph() {}
  bool has_node(int id) const { return m_nodeid_to_node_map.contains(id); }
  const GraphNode& get_node_by_id(int id) const;
  auto get_nodes() const {
    return m_nodeid_to_node_map |
           std::views::transform([](const auto& pair) -> const GraphNode& {
             return *pair.second;
           });
  }
  auto get_nodes_ids() const {
    return m_nodeid_to_node_map |
           std::views::transform(
               [](const auto& pair) -> int { return pair.first; });
  }
  auto get_edges_of_node(int node_id) const {
    if (!has_node(node_id))
      throw std::runtime_error("Graph::get_edges: node not found");
    return m_nodeid_to_edgesid.at(node_id) |
           std::views::transform([this](int edge_id) -> const GraphEdge& {
             return *m_edgeid_to_edge_map.at(edge_id);
           });
  }
  const GraphNode& add_node(int id);
  const GraphNode& add_node();
  const GraphEdge& add_edge(int from_id, int to_id);
  bool has_edge(int from_id, int to_id) const;
  void add_undirected_edge(int from_id, int to_id);
  int size() const { return m_nodeid_to_node_map.size(); }
  int get_number_of_edges() const;
  int get_edge_id(int from_id, int to_id) const;
  auto get_edges() const {
    return m_edgeid_to_edge_map |
           std::views::transform([](const auto& pair) -> const GraphEdge& {
             return *pair.second;
           });
  }
  int get_degree_of_node(int node_id) const;
  int get_in_degree_of_node(int node_id) const;
  const GraphEdge& get_edge(int from_id, int to_id) const;
  const GraphEdge& get_edge_by_id(int edge_id) const;
  void remove_node(int id);
  void remove_edge(int from_id, int to_id);
  void remove_undirected_edge(int from_id, int to_id);
  const std::string to_string() const;
  void print() const { std::cout << to_string() << std::endl; }
  // delete move constructor and move assignment operator
  Graph(const Graph&) = delete;
  Graph& operator=(const Graph&) = delete;
  // delete copy constructor and copy assignment operator
  Graph(Graph&&) = delete;
  Graph& operator=(Graph&&) = delete;
};

class GraphNode {
 private:
  int m_id;
  const Graph& m_graph_owner;

 public:
  GraphNode(int id, const Graph& owner) : m_id(id), m_graph_owner(owner) {}
  int get_id() const { return m_id; }
  std::string to_string() const;
  auto get_edges() const { return m_graph_owner.get_edges_of_node(m_id); };
  int get_degree() const { return get_edges().size(); }
  int get_in_degree() const {
    return m_graph_owner.get_in_degree_of_node(m_id);
  }
  void print() const { std::cout << to_string() << std::endl; }
  const Graph& get_owner() const { return m_graph_owner; }
};

using GraphEdgeHashSet = std::unordered_set<std::pair<int, int>, int_pair_hash>;

template <typename T>
using GraphEdgeHashMap =
    std::unordered_map<std::pair<int, int>, T, int_pair_hash>;

#endif
