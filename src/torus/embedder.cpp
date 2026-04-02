#include "domus/torus/embedder.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "embed_two_cycles.hpp"
#include "faces.hpp"

#include "domus/torus/type_4.hpp"

namespace domus::torus {
using namespace domus::graph;

std::optional<graph::Embedding> compute_toroidal_embedding(
    const Graph& graph, const Cycle& cycle_1, Cycle& cycle_2, const size_t intersection_node_id
) {
    auto [embedding, face] =
        compute_embedding_of_two_cycles(graph, cycle_1, cycle_2, intersection_node_id);

    if (face.type() == FaceType::TYPE_4) {
        handle_type_4(graph, embedding);
    } else {
        DOMUS_ASSERT(
            face.type() == FaceType::TYPE_3,
            "compute_toroidal_embedding: expected TYPE_3 face"
        );
        // TODO: handle type 3
    }

    return std::nullopt;
}

std::optional<graph::Embedding> compute_toroidal_embedding(const Graph& graph) {
    auto cycle_basis = algorithms::compute_cycle_basis(graph);
    for (size_t i = 0; i < cycle_basis.size(); ++i) {
        Cycle& cycle_1 = cycle_basis[i];
        for (size_t j = i + 1; j < cycle_basis.size(); ++j) {
            Cycle& cycle_2 = cycle_basis[j];
            std::optional<size_t> intersection_node_id =
                algorithms::do_cycles_intersect(cycle_1, cycle_2);
            if (!intersection_node_id.has_value())
                continue;
            auto embedding =
                compute_toroidal_embedding(graph, cycle_1, cycle_2, *intersection_node_id);
            if (embedding)
                return embedding;
        }
    }
    return std::nullopt;
}

} // namespace domus::torus