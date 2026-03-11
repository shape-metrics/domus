#pragma once

#include "domus/core/graph/graph.hpp"

Graph generate_connected_random_graph_degree_max_4(size_t number_of_nodes, size_t number_of_edges);

Graph generate_connected_random_graph(size_t number_of_nodes, size_t number_of_edges);

// n*m grid, n, m > 1
Graph generate_grid_graph(size_t n, size_t m);

// num_nodes > 1
Graph generate_triangle_graph(size_t num_nodes);