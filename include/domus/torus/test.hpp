#pragma once

#include <filesystem>
#include <vector>

#include "domus/core/graph/embedding.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::torus::test {

void test_all_possible_embeddings(const graph::Graph& graph);

bool is_toroidal_ground_truth(const graph::Graph& graph);

bool compare_with_ground_truth(const std::vector<graph::Graph>& graphs);

bool is_minimal_obstruction(const graph::Graph& graph);

namespace GraphType {
struct Planar {
    graph::Embedding embedding;
};
struct Toroidal {
    graph::Embedding embedding;
};
struct NonToroidal {};
struct MinimalNonToroidal {};
struct NonBiconnected {};
struct Uncomputed {};
} // namespace GraphType

using ObstructionResult = std::variant<
    GraphType::Planar,
    GraphType::Toroidal,
    GraphType::NonToroidal,
    GraphType::MinimalNonToroidal,
    GraphType::NonBiconnected,
    GraphType::Uncomputed>;

std::vector<ObstructionResult> find_minimal_obstructions(const std::vector<graph::Graph>& graphs);

void save_all_results(
    const std::vector<graph::Graph>& graphs,
    const std::vector<ObstructionResult>& results,
    const std::filesystem::path& obstructions_directory,
    bool check_correctness
);

} // namespace domus::torus::test