#include "test_graphs.hpp"

#include "domus/core/graph/generators.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::graph::test {

std::vector<Graph> forbidden_minors{generators::generate_k_n(5), generators::generate_k_n_m(3, 3)};

std::vector<Graph> two_cycle_graphs{
    []() {
        Graph graph;
        for (size_t i = 0; i < 7; ++i)
            graph.add_node();

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 3);
        graph.add_edge(3, 0);

        graph.add_edge(0, 4);
        graph.add_edge(4, 5);
        graph.add_edge(5, 6);
        graph.add_edge(6, 0);
        return graph;
    }(),
    []() {
        Graph graph;
        for (size_t i = 0; i < 10; ++i)
            graph.add_node();

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 3);

        graph.add_edge(3, 4);
        graph.add_edge(4, 5);
        graph.add_edge(5, 6);
        graph.add_edge(6, 0);

        graph.add_edge(0, 7);
        graph.add_edge(7, 8);
        graph.add_edge(8, 9);
        graph.add_edge(9, 3);
        return graph;
    }(),
    []() {
        Graph graph;
        for (size_t i = 0; i < 5; ++i)
            graph.add_node();

        graph.add_edge(0, 1);
        graph.add_edge(1, 2);
        graph.add_edge(2, 3);

        graph.add_edge(0, 3);

        graph.add_edge(0, 4);
        graph.add_edge(4, 3);
        return graph;
    }()
};

std::vector<Graph> planar_graphs{[]() {
    Graph graph;
    graph.add_node();
    graph.add_node();
    graph.add_node();
    graph.add_node();
    graph.add_node();
    graph.add_edge(0, 1);
    graph.add_edge(0, 2);
    graph.add_edge(1, 2);
    graph.add_edge(1, 3);
    graph.add_edge(2, 3);
    graph.add_edge(2, 4);
    graph.add_edge(3, 0);
    graph.add_edge(3, 4);
    graph.add_edge(4, 1);
    return graph;
}()};

} // namespace domus::graph::test