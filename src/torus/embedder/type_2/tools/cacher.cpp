#include "cacher.hpp"

#include "adjacencies.hpp"
#include "domus/core/debug.hpp"
#include "domus/core/graph/embedding.hpp"

namespace domus::torus {

std::optional<EmbeddingsHandler> EmbeddingsHandler::build(
    const NodesPositions& nodes_positions, Adjacencies& adjacencies, const graph::Embedding& partial
) {
    EmbeddingsHandler handler;

    // special faces
    for (size_t face_index = 0; face_index < adjacencies.get_faces().size(); ++face_index) {
        const Face& face = adjacencies.get_faces()[face_index];
        if (face.type() == FaceType::TYPE_1)
            continue;
        std::vector<const Bridge*> special_pieces;
        for (size_t piece_index : adjacencies.special_pieces_in_face(face_index))
            special_pieces.push_back(&adjacencies.get_pieces()[piece_index]);

        auto cylinder = PlanarizedCylinder::build(
            face_index,
            adjacencies.get_faces(),
            partial,
            {},
            special_pieces
        );
        if (!cylinder.has_value())
            return std::nullopt;
        handler.m_cylinder_embeddings.push_back(std::move(*cylinder));
    }

    // ordinary pieces
    for (size_t piece_index : adjacencies.all_ordinary_pieces()) {
        const PieceAdjacency& adjacency = adjacencies.get_adjacency(piece_index);
        const Bridge& piece = adjacencies.get_pieces()[piece_index];
        const std::vector<size_t> adjacent_faces = adjacency.adjacent_faces;
        for (size_t face_index : adjacent_faces) {
            auto embedding = CachedOrdinaryEmbedding::cache_ordinary_embedding(
                face_index,
                piece,
                nodes_positions
            );
            if (!embedding.has_value()) {
                adjacencies.remove_ordinary_piece_adjacency(piece_index, face_index);
                if (adjacencies.get_adjacency(piece_index).adjacent_faces.empty())
                    return std::nullopt;
                continue;
            }
            DOMUS_ASSERT(
                [&]() {
                    graph::Embedding copy = partial;
                    embedding->insert_into_face(
                        copy,
                        adjacencies.get_faces()[face_index],
                        face_index,
                        nodes_positions
                    );
                    return (graph::compute_embedding_genus(copy) == 1);
                }(),
                "EmbeddingsHandler::build: inserting the cached embedding in the "
                "total embedding changed the genus."
            );
            handler.add_cached_embedding(std::move(*embedding), piece_index);
        }
    }
    return handler;
}

const std::vector<PlanarizedCylinder>& EmbeddingsHandler::get_cylinder_embeddings() const {
    return m_cylinder_embeddings;
}

void EmbeddingsHandler::add_cached_embedding(
    CachedOrdinaryEmbedding&& embedding, size_t piece_index
) {
    if (m_cached_ordinary_embeddings.size() <= piece_index)
        m_cached_ordinary_embeddings.resize(piece_index + 1);
    m_cached_ordinary_embeddings[piece_index].push_back(std::move(embedding));
}

const CachedOrdinaryEmbedding&
EmbeddingsHandler::get_cached_embedding(size_t piece_index, size_t face_index) const {
    for (const auto& embedding : m_cached_ordinary_embeddings[piece_index])
        if (embedding.get_embedded_face_index() == face_index)
            return embedding;
    DOMUS_ASSERT(
        false,
        "EmbeddingsHandler::get_cached_embedding: did not find cached embedding for piece {} and "
        "face {}",
        piece_index,
        face_index
    );
    std::unreachable();
}

} // namespace domus::torus