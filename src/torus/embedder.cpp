#include "domus/torus/embedder.hpp"

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "../core/domus_debug.hpp"
#include "domus/planarity/embedding.hpp"

namespace domus::torus {
using namespace domus::graph;

enum class FaceType {
    TYPE_1,
    TYPE_2,
    TYPE_3,
    TYPE_4,
};

// TODO
FaceType compute_type_of_face(const Path& path);

// TODO
planarity::Embedding compute_embedding_of_two_cycles(const Cycle& cycle_1, const Cycle& cycle_2);

std::optional<planarity::Embedding>
compute_toroidal_embedding(const Graph& graph, const Cycle& cycle_1, const Cycle& cycle_2) {
    auto embedding = compute_embedding_of_two_cycles(cycle_1, cycle_2);
    DOMUS_ASSERT(
        planarity::compute_embedding_genus(embedding) == 1,
        "compute_toroidal_embedding: the embedding of the two cycles is not toroidal"
    );
    auto faces = planarity::compute_faces_in_embedding(embedding);
    DOMUS_ASSERT(
        faces.size() == 1,
        "compute_toroidal_embedding: should have obtained a single face"
    );
    const Path& path = faces[0];
    FaceType face_type = compute_type_of_face(path);
    DOMUS_ASSERT(
        face_type == FaceType::TYPE_3 || face_type == FaceType::TYPE_4,
        "compute_toroidal_embedding: face from cycles should be type 3 or 4"
    );
}

std::optional<planarity::Embedding> compute_toroidal_embedding(const Graph& graph) {
    auto cycle_basis = algorithms::compute_cycle_basis(graph);
    for (size_t i = 0; i < cycle_basis.size(); ++i) {
        const Cycle& cycle_1 = cycle_basis[i];
        for (size_t j = i + 1; j < cycle_basis.size(); ++j) {
            const Cycle& cycle_2 = cycle_basis[j];
            if (algorithms::do_cycles_intersect(cycle_1, cycle_2)) {
                auto embedding = compute_toroidal_embedding(graph, cycle_1, cycle_2);
                if (embedding)
                    return embedding;
            }
        }
    }
    return std::nullopt;
}

} // namespace domus::torus