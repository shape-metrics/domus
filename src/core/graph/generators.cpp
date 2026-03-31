#include "domus/core/graph/generators.hpp"

#include <format>
#include <iterator>
#include <stdlib.h>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::graph::generators {

Graph generate_connected_random_graph_degree_max_4(
    const size_t number_of_nodes, const size_t number_of_edges
) {
    DOMUS_ASSERT(
        number_of_edges <= 2 * number_of_nodes,
        "generate_connected_random_graph_degree_max_4: number of edges is too large."
    );
    DOMUS_ASSERT(
        number_of_edges + 1 >= number_of_nodes,
        "generate_connected_random_graph_degree_max_4: number of edges is too small."
    );
    Graph graph;
    for (size_t i = 0; i < number_of_nodes; ++i)
        graph.add_node();
    size_t added_edges = 0;
    while (added_edges < number_of_edges) {
        size_t i = static_cast<size_t>(rand()) % number_of_nodes;
        size_t j = static_cast<size_t>(rand()) % number_of_nodes;
        if (i == j || graph.are_neighbors(i, j))
            continue;
        if (graph.get_degree_of_node(i) >= 4u)
            continue;
        if (graph.get_degree_of_node(j) >= 4u)
            continue;
        graph.add_edge(i, j);
        ++added_edges;
    }
    if (!algorithms::is_graph_connected(graph))
        return generate_connected_random_graph_degree_max_4(number_of_nodes, number_of_edges);
    return graph;
}

Graph generate_connected_random_graph(size_t number_of_nodes, size_t number_of_edges) {
    DOMUS_ASSERT(
        number_of_edges + 1 >= number_of_nodes,
        "generate_connected_random_graph: number of edges is too small."
    );
    Graph graph;
    for (size_t i = 0; i < number_of_nodes; ++i)
        graph.add_node();
    size_t added_edges = 0;
    while (added_edges < number_of_edges) {
        size_t i = static_cast<size_t>(rand()) % number_of_nodes;
        size_t j = static_cast<size_t>(rand()) % number_of_nodes;
        if (i == j || graph.are_neighbors(i, j))
            continue;
        graph.add_edge(i, j);
        ++added_edges;
    }
    if (!algorithms::is_graph_connected(graph))
        return generate_connected_random_graph(number_of_nodes, number_of_edges);
    return graph;
}

// n*m grid, n, m > 1
Graph generate_grid_graph(size_t n, size_t m) {
    size_t num_nodes = 2 * n + 2 * m - 4;
    Graph graph;
    for (size_t i = 0; i < num_nodes; ++i)
        graph.add_node();
    for (size_t i = 0; i < num_nodes - 1; ++i)
        graph.add_edge(i, i + 1);
    graph.add_edge(0, num_nodes - 1);
    for (size_t i = 1; i < n - 1; ++i)
        graph.add_edge(i, 2 * n + m - i - 3);
    m -= 2;
    for (size_t i = 0; i < m; ++i)
        graph.add_edge(n + i, 2 * n + 2 * m - i - 1);
    return graph;
}

// num_nodes > 1
Graph generate_triangle_graph(size_t num_nodes) {
    num_nodes = 3 * num_nodes;
    Graph graph;
    for (size_t i = 0; i < num_nodes; ++i)
        graph.add_node();
    for (size_t i = 0; i < num_nodes - 3; ++i) {
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

Graph generate_k_n(size_t n) {
    Graph graph;
    for (size_t i = 0; i < n; ++i)
        graph.add_node();
    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            graph.add_edge(i, j);
    return graph;
}

Graph generate_k_n_m(size_t n, size_t m) {
    Graph graph;
    for (size_t i = 0; i < n + m; ++i)
        graph.add_node();
    for (size_t i = 0; i < n; ++i)
        for (size_t j = n; j < n + m; ++j)
            graph.add_edge(i, j);
    return graph;
}

std::string code_to_generate_graph(const Graph& graph) {
    std::string code;
    auto out = std::back_inserter(code);
    std::format_to(out, "Graph graph;\n");

    for (size_t i = 0; i < graph.get_number_of_nodes(); ++i)
        std::format_to(out, "graph.add_node();\n");

    for (size_t node_id : graph.get_nodes_ids())
        for (EdgeIter edge : graph.get_out_edges(node_id))
            std::format_to(out, "graph.add_edge({}, {});\n", node_id, edge.neighbor_id);

    return code;
}

} // namespace domus::graph::generators