#include "domus/torus/embedder.hpp"

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

namespace domus::torus {
using namespace domus::graph;

bool do_cycles_intersect(const Cycle& cycle_1, const Cycle& cycle_2) {
    std::vector<size_t> nodes_in_cycle_1;
    std::vector<size_t> nodes_in_cycle_2;
}

std::optional<planarity::Embedding> compute_toroidal_embedding(const Graph& graph) {
    auto cycle_basis = algorithms::compute_cycle_basis(graph);

    return std::nullopt;
}

} // namespace domus::torus