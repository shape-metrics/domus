#pragma once

#include "domus/core/graph/graph.hpp"

namespace domus::graph::generators {

Graph generate_connected_random_graph_degree_max_4(size_t number_of_nodes, size_t number_of_edges);

Graph generate_connected_random_graph(size_t number_of_nodes, size_t number_of_edges);

// n*m grid, n, m > 1
Graph generate_grid_graph(size_t n, size_t m);

// num_nodes > 1
Graph generate_triangle_graph(size_t num_nodes);

// clique of size n
Graph generate_k_n(size_t n);

// complete bipartite graph n x m
Graph generate_k_n_m(size_t n, size_t m);

std::string code_to_generate_graph(const Graph& graph);

} // namespace domus::graph::generators