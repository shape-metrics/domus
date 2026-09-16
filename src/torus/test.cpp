#include "domus/torus/test.hpp"

#include "domus/core/graph/embedding.hpp"

#include "faces.hpp"

namespace domus::torus::test {
using namespace graph;

void test_all_possible_embeddings(const Graph& graph) {
    std::vector<Embedding> embeddings = compute_all_possible_embeddings(graph);
    std::println("Number of embeddings: {}", embeddings.size());
    std::vector<size_t> genuses;
    std::vector<size_t> max_face_types;
    for (const Embedding& embedding : embeddings) {
        const std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
        const size_t g = compute_embedding_genus(
            graph.get_number_of_nodes(),
            graph.get_number_of_edges(),
            faces.size(),
            1
        );
        if (genuses.size() <= g)
            genuses.resize(g + 1);
        ++genuses[g];
        if (g == 1) {
            size_t max_face_type = 0;
            for (FaceType type : std::views::transform(faces, [&](const Path& path) {
                     return compute_face_from_path(Path(path), graph).type();
                 })) {
                if (static_cast<size_t>(type) > max_face_type)
                    max_face_type = static_cast<size_t>(type);
            }

            if (max_face_types.size() <= static_cast<size_t>(max_face_type))
                max_face_types.resize(static_cast<size_t>(max_face_type) + 1);
            ++max_face_types[static_cast<size_t>(max_face_type)];
        }
    }

    for (size_t i = 1; i < genuses.size(); ++i) {
        std::println("Genus: [{:>2}] Quantity: [{:>4}]", i, genuses[i]);
    }

    for (size_t i = 1; i < max_face_types.size(); ++i) {
        std::println("Case: [{:>2}] Quantity: [{:>4}]", i, max_face_types[i]);
    }
}

} // namespace domus::torus::test