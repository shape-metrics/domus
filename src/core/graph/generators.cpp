#include "domus/core/graph/generators.hpp"

#include <stdlib.h>

#include "domus/core/graph/graphs_algorithms.hpp"

using namespace std;

expected<UndirectedGraph, string> generate_connected_random_graph_degree_max_4(
    const size_t number_of_nodes, const size_t number_of_edges
) {
    if (number_of_edges > 2 * number_of_nodes) {
        string error = "Error in generate_connected_random_graph_degree_max_4: "
                       "Number of edges is too large.\n";
        error += "Number of nodes: " + to_string(number_of_nodes);
        error += "\nNumber of edges: " + to_string(number_of_edges);
        return std::unexpected(error);
    }
    if (number_of_edges + 1 < number_of_nodes) {
        string error = "Error in generate_connected_random_graph_degree_max_4: "
                       "Number of edges is too small.\n";
        error += "Number of nodes: " + to_string(number_of_nodes);
        error += "\nNumber of edges: " + to_string(number_of_edges);
        return std::unexpected(error);
    }
    UndirectedGraph graph;
    for (size_t i = 0; i < number_of_nodes; ++i)
        graph.add_node(static_cast<int>(i));
    size_t added_edges = 0;
    while (added_edges < number_of_edges) {
        const int i = rand() % static_cast<int>(number_of_nodes);
        const int j = rand() % static_cast<int>(number_of_nodes);
        if (i == j || graph.has_edge(i, j))
            continue;
        if (graph.get_degree_of_node(i) >= 4)
            continue;
        if (graph.get_degree_of_node(j) >= 4)
            continue;
        graph.add_edge(i, j);
        ++added_edges;
    }
    if (!is_graph_connected(graph))
        return generate_connected_random_graph_degree_max_4(number_of_nodes, number_of_edges);
    return graph;
}

expected<UndirectedGraph, string>
generate_connected_random_graph(const size_t number_of_nodes, const size_t number_of_edges) {
    if (number_of_edges + 1 < number_of_nodes) {
        string error = "Error in generate_connected_random_graph: "
                       "Number of edges is too small.\n";
        error += "Number of nodes: " + to_string(number_of_nodes);
        error += "\nNumber of edges: " + to_string(number_of_edges);
        return std::unexpected(error);
    }
    UndirectedGraph graph;
    for (int i = 0; i < static_cast<int>(number_of_nodes); ++i)
        graph.add_node(i);
    size_t added_edges = 0;
    while (added_edges < number_of_edges) {
        int i = rand() % static_cast<int>(number_of_nodes);
        int j = rand() % static_cast<int>(number_of_nodes);
        if (i == j || graph.has_edge(i, j))
            continue;
        graph.add_edge(i, j);
        ++added_edges;
    }
    if (!is_graph_connected(graph))
        return generate_connected_random_graph(number_of_nodes, number_of_edges);
    return graph;
}

// n*m grid, n, m > 1
UndirectedGraph generate_grid_graph(const size_t n, const size_t m) {
    const int num_nodes = 2 * static_cast<int>(n) + 2 * static_cast<int>(m) - 4;
    const int n_int = static_cast<int>(n);
    int m_int = static_cast<int>(m);
    UndirectedGraph graph;
    for (int i = 0; i < num_nodes; ++i)
        graph.add_node(i);
    for (int i = 0; i < num_nodes - 1; ++i)
        graph.add_edge(i, i + 1);
    graph.add_edge(0, num_nodes - 1);
    for (int i = 1; i < n_int - 1; ++i)
        graph.add_edge(i, 2 * n_int + m_int - i - 3);
    m_int -= 2;
    for (int i = 0; i < m_int; ++i)
        graph.add_edge(n_int + i, 2 * n_int + 2 * m_int - i - 1);
    return graph;
}

// num_nodes > 1
UndirectedGraph generate_triangle_graph(size_t num_nodes) {
    num_nodes = 3 * num_nodes;
    UndirectedGraph graph;
    for (int i = 0; i < static_cast<int>(num_nodes); ++i)
        graph.add_node(i);
    for (int i = 0; i < static_cast<int>(num_nodes) - 3; ++i) {
        if (i % 3 == 2) {
            graph.add_edge(i, i + 3);
            graph.add_edge(i + 3, i - 2);
        } else {
            graph.add_edge(i, i + 3);
            graph.add_edge(i + 1, i + 3);
        }
    }
    return graph;
}