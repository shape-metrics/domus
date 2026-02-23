#ifndef MY_GRAPH_GENERATORS_H
#define MY_GRAPH_GENERATORS_H

#include <stddef.h>

#include "domus/core/graph/graph.hpp"

UndirectedGraph
generate_connected_random_graph_degree_max_4(size_t number_of_nodes, size_t number_of_edges);

UndirectedGraph generate_connected_random_graph(size_t number_of_nodes, size_t number_of_edges);

// n*m grid, n, m > 1
UndirectedGraph generate_grid_graph(size_t n, size_t m);

// num_nodes > 1
UndirectedGraph generate_triangle_graph(size_t num_nodes);

#endif