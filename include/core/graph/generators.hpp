#ifndef MY_GRAPH_GENERATORS_H
#define MY_GRAPH_GENERATORS_H

#include <memory>

#include "core/graph/graph.hpp"

std::unique_ptr<Graph> generate_connected_random_graph_degree_max_4(
    int number_of_nodes, int number_of_edges);

std::unique_ptr<Graph> generate_connected_random_graph(int number_of_nodes,
                                                       int number_of_edges);

// n*m grid, n, m > 1
std::unique_ptr<Graph> generate_grid_graph(int n, int m);

// num_nodes > 1
std::unique_ptr<Graph> generate_triangle_graph(int num_nodes);

#endif