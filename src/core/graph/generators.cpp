#include "core/graph/generators.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <unordered_set>

#include "core/graph/graphs_algorithms.hpp"

std::unique_ptr<Graph> generate_connected_random_graph_degree_max_4(
    int number_of_nodes, int number_of_edges) {
  if (number_of_edges > 2 * number_of_nodes)
    throw std::runtime_error("Number of edges is too large");
  if (number_of_edges < number_of_nodes - 1)
    throw std::runtime_error("Number of edges is too small");
  auto graph = std::make_unique<Graph>();
  for (int i = 0; i < number_of_nodes; ++i) graph->add_node(i);
  int added_edges = 0;
  while (added_edges < number_of_edges) {
    int i = rand() % number_of_nodes;
    int j = rand() % number_of_nodes;
    if (i == j || graph->has_edge(i, j)) continue;
    if (graph->get_node_by_id(i).get_degree() >= 4) continue;
    if (graph->get_node_by_id(j).get_degree() >= 4) continue;
    graph->add_undirected_edge(i, j);
    ++added_edges;
  }
  if (!is_graph_connected(*graph))
    return generate_connected_random_graph_degree_max_4(number_of_nodes,
                                                        number_of_edges);
  return graph;
}

std::unique_ptr<Graph> generate_connected_random_graph(int number_of_nodes,
                                                       int number_of_edges) {
  if (number_of_edges < number_of_nodes - 1)
    throw std::runtime_error("Number of edges is too small");
  auto graph = std::make_unique<Graph>();
  for (int i = 0; i < number_of_nodes; ++i) graph->add_node(i);
  int added_edges = 0;
  while (added_edges < number_of_edges) {
    int i = rand() % number_of_nodes;
    int j = rand() % number_of_nodes;
    if (i == j || graph->has_edge(i, j)) continue;
    graph->add_undirected_edge(i, j);
    ++added_edges;
  }
  if (!is_graph_connected(*graph))
    return generate_connected_random_graph(number_of_nodes, number_of_edges);
  return graph;
}

// n*m grid, n, m > 1
std::unique_ptr<Graph> generate_grid_graph(int n, int m) {
  int num_nodes = 2 * n + 2 * m - 4;
  auto graph = std::make_unique<Graph>();
  for (int i = 0; i < num_nodes; ++i) graph->add_node(i);
  for (int i = 0; i < num_nodes - 1; ++i) graph->add_undirected_edge(i, i + 1);
  graph->add_undirected_edge(0, num_nodes - 1);
  for (int i = 1; i < n - 1; ++i)
    graph->add_undirected_edge(i, (2 * n) + m - i - 3);
  m -= 2;
  for (int i = 0; i < m; ++i)
    graph->add_undirected_edge(n + i, (2 * n) + (2 * m) - i - 1);
  return graph;
}

// num_nodes > 1
std::unique_ptr<Graph> generate_triangle_graph(int num_nodes) {
  num_nodes = 3 * num_nodes;
  auto graph = std::make_unique<Graph>();
  for (int i = 0; i < num_nodes; ++i) graph->add_node(i);
  for (int i = 0; i < num_nodes - 3; ++i) {
    if (i % 3 == 2) {
      graph->add_undirected_edge(i, i + 3);
      graph->add_undirected_edge(i + 3, i - 2);
    } else {
      graph->add_undirected_edge(i, i + 3);
      graph->add_undirected_edge(i + 1, i + 3);
    }
  }
  return graph;
}