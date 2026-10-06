#pragma once

#include <vector>

namespace domus::graph {
class Graph;
}

namespace domus::torus::test {

void test_all_possible_embeddings(const graph::Graph& graph);

bool is_toroidal_ground_truth(const graph::Graph& graph);

bool compare_with_ground_truth(const std::vector<graph::Graph>& graphs);

bool is_minimal_obstruction(const graph::Graph& graph);

std::vector<std::pair<graph::Graph, bool>>
find_minimal_obstructions(const std::vector<graph::Graph>& graphs);

} // namespace domus::torus::test