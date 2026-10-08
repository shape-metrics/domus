#include "adjacencies.hpp"
#include "domus/core/graph/embedding.hpp"
#include "nodes_positions.hpp"

#include <algorithm>

namespace domus::torus {
using namespace domus::graph;

const std::vector<std::vector<size_t>> Adjacencies::compute_nodes_to_faces() {
    std::vector<std::vector<size_t>> nodes_to_faces(m_graph.get_number_of_nodes());
    for (size_t face_id = 0; face_id < m_faces.size(); ++face_id) {
        const Face& face = m_faces[face_id];
        for (size_t j = 0; j < face.path().number_of_nodes() - 1; ++j) {
            const size_t node_id = face.path().get_node_id_at_position(j);
            if (!std::ranges::contains(nodes_to_faces[node_id], face_id))
                nodes_to_faces[node_id].push_back(face_id);
        }
    }
    return nodes_to_faces;
}

bool Adjacencies::compute_piece_to_adjacent_faces(
    const std::vector<std::vector<size_t>>& nodes_to_faces
) {
    m_pieces_adjacencies.reserve(m_pieces.size());
    m_is_across_in_a_cylinder.reserve(m_pieces.size());
    m_is_ordinary.resize(m_pieces.size(), false);
    m_faces_adjacencies.resize(m_faces.size());

    for (size_t bridge_index = 0; bridge_index < m_pieces.size(); bridge_index++) {
        const Bridge& bridge = m_pieces[bridge_index];
        std::vector<std::vector<size_t>> faces;
        for (const size_t old_attachment_id : bridge.get_old_attachments())
            faces.push_back(nodes_to_faces[old_attachment_id]);

        std::vector<size_t> common_faces;
        for (const size_t face_id : faces[0]) {
            if (std::ranges::contains(common_faces, face_id))
                continue;

            const bool in_all = std::all_of(
                faces.begin() + 1,
                faces.end(),
                [&](const std::vector<size_t>& face_list) {
                    return std::ranges::contains(face_list, face_id);
                }
            );

            if (in_all)
                common_faces.push_back(face_id);
        }
        std::ranges::sort(common_faces);

        if (common_faces.empty())
            return false;

        std::optional<size_t> special_face_id;
        for (const size_t face_id : common_faces) {
            if (m_faces[face_id].type() == FaceType::TYPE_2) {
                for (const size_t old_attachment_id : bridge.get_old_attachments()) {
                    if (m_faces[face_id]
                            .is_node_repeated_in_face()
                            .get_label(old_attachment_id)
                            .test(0)) {
                        special_face_id = face_id;
                        break;
                    }
                }
            }
            if (special_face_id.has_value())
                break;
        }

        const bool is_special_piece = special_face_id.has_value();

        m_pieces_adjacencies.push_back(
            PieceAdjacency{
                .type = (is_special_piece) ? PieceAdjacency::PieceType::SPECIAL
                                           : PieceAdjacency::PieceType::ORDINARY,
                .adjacent_faces = common_faces,
            }
        );

        bool is_across = false;
        if (!is_special_piece) {
            m_is_ordinary[bridge_index] = true;
            for (const size_t face_index : m_pieces_adjacencies.back().adjacent_faces) {
                if (is_ordinary_piece_cutting_cylinder(bridge_index, face_index)) {
                    is_across = true;
                    break;
                }
            }
        }
        m_is_across_in_a_cylinder.push_back(is_across);

        if (is_special_piece) {
            m_faces_adjacencies[special_face_id.value()].special_pieces.push_back(bridge_index);
        } else {
            for (const size_t face_index : m_pieces_adjacencies.back().adjacent_faces)
                m_faces_adjacencies[face_index].ordinary_pieces.push_back(bridge_index);
        }
    }

    for (size_t piece_index = 0; piece_index < m_pieces.size(); piece_index++)
        if (m_is_ordinary[piece_index])
            m_ordinary_pieces.push_back(piece_index);

    return true;
}

void Adjacencies::compute_cylinders_boundary_components() {
    m_cylinder_components.assign(m_faces.size(), std::nullopt);
    for (size_t face_index = 0; face_index < m_faces.size(); face_index++) {
        const Face& face = m_faces[face_index];
        if (face.type() == FaceType::TYPE_1)
            continue;
        DOMUS_ASSERT(
            face.type() == FaceType::TYPE_2,
            "Adjacencies::compute_cylinders_boundary_components: expected either type 1 or type 2 "
            "faces"
        );
        const Path& path = face.path();
        const auto& is_repeated = face.is_node_repeated_in_face();

        std::vector<std::vector<size_t>> components;
        std::vector<size_t> current_component;

        for (size_t i = 0; i < path.number_of_nodes() - 1; ++i) {
            const size_t u = path.get_node_id_at_position(i);
            if (!is_repeated.get_label(u).test(0)) {
                current_component.push_back(u);
            } else {
                if (!current_component.empty()) {
                    components.push_back(current_component);
                    current_component.clear();
                }
            }
        }
        if (!current_component.empty()) {
            components.push_back(current_component);
        }

        if (components.size() > 1) {
            const size_t first_node = path.get_node_id_at_position(0);
            const size_t last_node = path.get_node_id_at_position(path.number_of_nodes() - 2);
            if (!is_repeated.get_label(first_node).test(0) &&
                !is_repeated.get_label(last_node).test(0)) {
                components[0].insert(
                    components[0].end(),
                    components.back().begin(),
                    components.back().end()
                );
                components.pop_back();
            }
        }

        DOMUS_ASSERT(
            components.size() == 2,
            "Adjacencies::compute_cylinders_boundary_components: cylinder face must have exactly "
            "two non-repeated paths"
        );

        CylinderBoundaryComponents result;
        for (const size_t u : components[0])
            result.component_0.add_node(u);
        for (const size_t u : components[1])
            result.component_1.add_node(u);

        m_cylinder_components[face_index] = result;
    }
}

bool Adjacencies::is_ordinary_piece_cutting_cylinder(size_t p_index, size_t face_index) const {
    if (face_index >= m_cylinder_components.size() ||
        !m_cylinder_components[face_index].has_value())
        return false;
    const auto& comp = m_cylinder_components[face_index].value();
    bool has_attachment_in_c1 = false;
    bool has_attachment_in_c2 = false;
    for (const size_t u : m_pieces[p_index].get_old_attachments()) {
        if (comp.component_0.has_node(u))
            has_attachment_in_c1 = true;
        if (comp.component_1.has_node(u))
            has_attachment_in_c2 = true;
        if (has_attachment_in_c1 && has_attachment_in_c2)
            return true;
    }
    return false;
}

bool Adjacencies::compute_cache_embeddings() {
    NodesPositions positions(*this);
    m_cylinder_embeddings.resize(m_faces.size());

    // special faces
    for (size_t face_index = 0; face_index < m_faces.size(); ++face_index) {
        const Face& face = m_faces[face_index];
        if (face.type() == FaceType::TYPE_1)
            continue;
        m_number_of_cylinders++;
        std::vector<const Bridge*> special_pieces;
        for (size_t piece_index : special_pieces_in_face(face_index))
            special_pieces.push_back(&m_pieces[piece_index]);
        auto cylinder =
            PlanarizedCylinder::build(face_index, m_faces, m_embedding, {}, special_pieces);
        if (!cylinder.has_value())
            return false;
        m_cylinder_embeddings[face_index] = std::move(cylinder);
    }

    // ordinary pieces
    for (size_t piece_index = 0; piece_index < m_pieces.size(); piece_index++) {
        const PieceAdjacency& adjacency = get_adjacency(piece_index);
        if (adjacency.type == PieceAdjacency::PieceType::SPECIAL)
            continue;
        const Bridge& piece = m_pieces[piece_index];
        for (size_t face_index : get_adjacency(piece_index).adjacent_faces) {
            const Face& face = m_faces[face_index];

            auto embedding = CachedOrdinaryEmbedding::cache_ordinary_embedding(
                face_index,
                face,
                piece,
                positions
            );
            if (!embedding.has_value()) {
                remove_piece_adjacency(piece_index, face_index);
                if (get_adjacency(piece_index).adjacent_faces.empty())
                    return false;
                continue;
            }
            DOMUS_ASSERT(
                [&]() {
                    Embedding copy = m_embedding;
                    embedding->insert_into_embedding(copy);
                    return (compute_embedding_genus(copy) == 1);
                }(),
                "Adjacencies::compute_cache_embeddings: inserting the cached embedding in the "
                "total "
                "embedding changed the genus."
            );
            add_cached_embedding(std::move(*embedding), piece_index, face_index);
        }
    }
    return true;
}

Adjacencies::Adjacencies(
    const std::vector<Bridge>& pieces,
    const std::vector<Face>& faces,
    const graph::Graph& graph,
    const graph::Embedding& embedding
)
    : m_pieces(pieces), m_faces(faces), m_graph(graph), m_embedding(embedding) {}

std::optional<Adjacencies> Adjacencies::build_adjacencies(
    const std::vector<Bridge>& pieces,
    const std::vector<Face>& faces,
    const graph::Graph& graph,
    const graph::Embedding& embedding
) {
    Adjacencies adjacencies(pieces, faces, graph, embedding);
    adjacencies.compute_cylinders_boundary_components();
    if (adjacencies.compute_piece_to_adjacent_faces(adjacencies.compute_nodes_to_faces()))
        if (adjacencies.compute_cache_embeddings())
            return adjacencies;
    return std::nullopt;
}

const PieceAdjacency& Adjacencies::get_adjacency(size_t piece_index) const {
    return m_pieces_adjacencies[piece_index];
}

const std::vector<Bridge>& Adjacencies::get_pieces() const { return m_pieces; }

const std::vector<Face>& Adjacencies::get_faces() const { return m_faces; }

const graph::Graph& Adjacencies::get_graph() const { return m_graph; }

const std::vector<size_t>& Adjacencies::special_pieces_in_face(size_t face_index) const {
    return m_faces_adjacencies[face_index].special_pieces;
}

const std::vector<size_t>& Adjacencies::ordinary_pieces_in_face(size_t face_index) const {
    return m_faces_adjacencies[face_index].ordinary_pieces;
}

const std::vector<size_t>& Adjacencies::all_ordinary_pieces() const { return m_ordinary_pieces; }

bool Adjacencies::is_ordinary_piece(size_t piece_index) const { return m_is_ordinary[piece_index]; }

} // namespace domus::torus