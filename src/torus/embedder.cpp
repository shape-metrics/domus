#include "domus/torus/embedder.hpp"

#include <optional>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "cases/3/type_3.hpp"
#include "cases/4/type_4.hpp"
#include "embed_two_cycles.hpp"

namespace domus::torus {
using namespace domus::graph;

std::optional<Embedding> compute_toroidal_embedding(
    Graph& graph,
    const Cycle& cycle_1,
    Cycle& cycle_2,
    const size_t intersection_node_id,
    const size_t jolly_id
) {
    auto [embedding, face] =
        compute_embedding_of_two_cycles(graph, cycle_1, cycle_2, intersection_node_id);
    if (face.type() == FaceType::TYPE_4) {
        if (handle_type_4(graph, embedding, face, jolly_id)) {
            return embedding;
        }
        return std::nullopt;
    }
    DOMUS_ASSERT(
        face.type() == FaceType::TYPE_3,
        "compute_toroidal_embedding: face is neither of type 3 or 4"
    );
    if (handle_type_3(graph, embedding, face, jolly_id))
        return embedding;
    return std::nullopt;
}

std::optional<Embedding> compute_toroidal_embedding(const Graph& graph) {
    if (graph.get_number_of_edges() > 3 * graph.get_number_of_nodes())
        return std::nullopt;

    Graph graph_copy = graph;
    std::vector<Cycle> cycle_basis = algorithms::compute_cycle_basis(graph_copy);

    // adding jolly node used to insert new paths to split faces
    size_t jolly_id = graph_copy.add_node();

    // TODO replace cycle basis of the whole graph with cycle basis of k5/k33 subdivision
    for (size_t i = 0; i < cycle_basis.size(); ++i) {
        Cycle& cycle_1 = cycle_basis[i];
        for (size_t j = i + 1; j < cycle_basis.size(); ++j) {
            Cycle& cycle_2 = cycle_basis[j];
            std::optional<size_t> intersection_node_id =
                algorithms::do_cycles_intersect(cycle_1, cycle_2);
            if (!intersection_node_id.has_value())
                continue;
            std::optional<Embedding> embedding = compute_toroidal_embedding(
                graph_copy,
                cycle_1,
                cycle_2,
                *intersection_node_id,
                jolly_id
            );
            if (embedding.has_value())
                return embedding;
        }
    }
    return std::nullopt;
}

} // namespace domus::torus
