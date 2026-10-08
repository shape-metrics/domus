#pragma once

#include <optional>

#include "nodes_positions.hpp"
#include "ordinary_pieces.hpp"
#include "planarized_cylinder.hpp"

namespace domus::torus {

class EmbeddingsHandler {
    std::vector<PlanarizedCylinder> m_cylinder_embeddings;
    std::vector<std::vector<CachedOrdinaryEmbedding>> m_cached_ordinary_embeddings;

    EmbeddingsHandler() = default;

    void add_cached_embedding(CachedOrdinaryEmbedding&& embedding, size_t piece_index);

  public:
    static std::optional<EmbeddingsHandler> build(
        const NodesPositions& nodes_positions,
        Adjacencies& adjacencies,
        const graph::Embedding& partial
    );
    const std::vector<PlanarizedCylinder>& get_cylinder_embeddings() const;
    const CachedOrdinaryEmbedding&
    get_cached_embedding(size_t piece_index, size_t face_index) const;
};

} // namespace domus::torus