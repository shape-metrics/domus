#include "cacher.hpp"

#include "adjacencies.hpp"

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
                    Embedding copy = m_embedding;
                    embedding->insert_into_face(copy, m_faces[face_index]);
                    return (compute_embedding_genus(copy) == 1);
                }(),
                "Adjacencies::compute_cache_embeddings: inserting the cached embedding in the "
                "total embedding changed the genus."
            );
            handler.add_cached_embedding(std::move(*embedding), piece_index, face_index);
        }
    }
    return handler;
}

void EmbeddingsHandler::add_cached_embedding(
    CachedOrdinaryEmbedding&& embedding, size_t piece_index, size_t face_index
) {
    // TODO
}

} // namespace domus::torus