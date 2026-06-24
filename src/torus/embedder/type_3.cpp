#include "type_3.hpp"

#include <ranges>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/utils.hpp"

#include "../bridge.hpp"
#include "../faces.hpp"
#include "type_2.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

class SplitterWithPath {
    graph::Graph& m_graph;
    graph::Embedding& m_embedding;
    const Face& m_face;
    const size_t m_jolly_id;
    const std::vector<Bridge>& m_bridges;

    bool try_embedding_extension(const std::vector<Face>& faces);
    bool try_face_splits_with_path(const graph::Path& path);
    std::optional<std::vector<Face>> is_split_good(const std::vector<graph::Path>& faces);

  public:
    SplitterWithPath(
        graph::Graph& graph,
        graph::Embedding& embedding,
        const Face& face,
        size_t jolly_id,
        const std::vector<Bridge>& bridges
    );
    bool try_paths_inside_graph();
    bool try_edges_not_in_graph();
};

bool are_attachments_in_same_repeated_path(
    const NodesLabels<std::bitset<2>>& is_node_in_repeated_path,
    const size_t attachment_id_1,
    const size_t attachment_id_2
) {
    const std::bitset<2>& repeated_paths_1 = is_node_in_repeated_path.get_label(attachment_id_1);
    const std::bitset<2>& repeated_paths_2 = is_node_in_repeated_path.get_label(attachment_id_2);
    for (size_t i = 0; i < 2; i++)
        if (repeated_paths_1.test(i) && repeated_paths_2.test(i))
            return true;
    return false;
}

Path path_of_chord(const Graph& graph, const Bridge& chord) {
    DOMUS_ASSERT(
        chord.get_bridge().get_number_of_nodes() == 2,
        "path_of_chord: function only for bridges that are a single edge"
    );
    const size_t node_1 = chord.get_new_id_to_old_id().get_label(0);
    const size_t edge_id = chord.get_new_edge_id_to_old_id().get_label(0);
    Path path;
    path.push_back(graph, node_1, edge_id);
    return path;
}

void insert_path_in_embedding(Embedding& embedding, const Path& path) {
    for (size_t i = 0; i < path.number_of_edges(); ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.add_edge(node_id_1, node_id_2, edge_id);
        embedding.add_edge(node_id_2, node_id_1, edge_id);
    }
}

void remove_path_from_embedding(Embedding& embedding, const Path& path) {
    for (size_t i = 0; i < path.number_of_edges(); ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.remove_edge(node_id_1, node_id_2, edge_id);
        embedding.remove_edge(node_id_2, node_id_1, edge_id);
    }
}

inline auto all_pairs_of_view(auto&& feet) {
    auto feet_view = std::views::all(std::forward<decltype(feet)>(feet));
    return feet_view | std::views::enumerate |
           std::views::transform([feet_view](auto&& tuple) mutable {
               auto [i, first] = tuple;
               // for each element at i, create pairs with elements from i+1 to end
               return feet_view | std::views::drop(i + 1) |
                      std::views::transform([first](auto&& second) {
                          return std::make_pair(first, second);
                      });
           }) |
           std::views::join; // flatten
}

SplitterWithPath::SplitterWithPath(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    size_t jolly_id,
    const std::vector<Bridge>& bridges
)
    : m_graph(graph), m_embedding(embedding), m_face(face), m_jolly_id(jolly_id),
      m_bridges(bridges) {}

auto candidate_face_splitting_paths_in_bridge(
    const Bridge& bridge,
    const NodesLabels<std::bitset<2>>& is_node_in_repeated_path,
    const Graph& graph
) {
    return all_pairs_of_view(bridge.get_attachments()) | std::views::filter([&](const auto& pair) {
               const size_t attachment_1 = pair.first;
               const size_t attachment_2 = pair.second;
               const size_t old_attachment_1 =
                   bridge.get_new_id_to_old_id().get_label(attachment_1);
               const size_t old_attachment_2 =
                   bridge.get_new_id_to_old_id().get_label(attachment_2);

               return are_attachments_in_same_repeated_path(
                   is_node_in_repeated_path,
                   old_attachment_1,
                   old_attachment_2
               );
           }) |
           std::views::transform([&](const auto& pair) {
               Path path = algorithms::find_shortest_path_between_nodes(
                               bridge.get_bridge(),
                               pair.first,
                               pair.second
               )
                               .value();
               return path;
           }) |
           std::views::transform([&](const Path& path) {
               Path old_path;
               for (const auto [edge_id, prev_node_id] : path.get_edges()) {
                   const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(edge_id);
                   const size_t old_prev_node_id =
                       bridge.get_new_id_to_old_id().get_label(prev_node_id);
                   old_path.push_back(graph, old_prev_node_id, old_edge_id);
               }
               return old_path;
           });
}

bool SplitterWithPath::try_edges_not_in_graph() {
    for (size_t i = 0; i < m_face.path().number_of_nodes() - 1; i++) {
        const size_t node_id_1 = m_face.path().node_id_at_position(i);
        if (m_graph.get_degree_of_node(node_id_1) == 3)
            continue;
        for (size_t j = i + 1; j < m_face.path().number_of_nodes(); j++) {
            const size_t node_id_2 = m_face.path().node_id_at_position(j);
            if (m_graph.get_degree_of_node(node_id_2) == 3)
                continue;
            if (node_id_1 == node_id_2)
                continue;
            DOMUS_ASSERT(
                m_graph.get_degree_of_node(m_jolly_id) == 0,
                "FaceSplitter::try_edges_not_in_graph: no jolly node available"
            );

            const size_t edge_id_1 = m_graph.add_edge(node_id_1, m_jolly_id);
            const size_t edge_id_2 = m_graph.add_edge(m_jolly_id, node_id_2);

            Path path;
            path.push_back(m_graph, node_id_1, edge_id_1);
            path.push_back(m_graph, m_jolly_id, edge_id_2);
            if (try_face_splits_with_path(path))
                return true;

            m_graph.remove_edge(edge_id_1);
            m_graph.remove_edge(edge_id_2);
        }
    }
    return false;
}

bool SplitterWithPath::try_paths_inside_graph() {
    for (const Bridge& bridge : m_bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(m_graph, bridge);
            if (try_face_splits_with_path(path))
                return true;
        } else {
            for (const Path& path : candidate_face_splitting_paths_in_bridge(
                     bridge,
                     m_face.is_node_in_repeated_path(),
                     m_graph
                 ))
                if (try_face_splits_with_path(path))
                    return true;
        }
    }
    return false;
}

std::optional<std::vector<Face>> SplitterWithPath::is_split_good(const std::vector<Path>& paths) {
    size_t isolated_nodes = 0;
    for (const size_t node_id : m_embedding.get_nodes_ids())
        if (m_embedding.get_degree_of_node(node_id) == 0)
            isolated_nodes++;
    if (compute_embedding_genus(
            m_embedding.get_number_of_nodes(),
            m_embedding.get_number_of_edges(),
            paths.size(),
            1 + isolated_nodes
        ) > 1)
        return std::nullopt;
    std::vector<Face> faces;
    for (Path path : paths)
        faces.push_back(compute_face_from_path(std::move(path), m_graph));
    for (auto& face : faces)
        if (face.type() == FaceType::TYPE_3)
            return std::nullopt;
    return faces;
}

bool SplitterWithPath::try_face_splits_with_path(const Path& path) {
    DOMUS_ASSERT(
        compute_embedding_genus(m_embedding) == 1,
        "FaceSplitter::try_face_splits_with_path: initial genus of embedding is not 1"
    );
    const size_t first_id = path.get_first_node_id();
    const size_t last_id = path.get_last_node_id();

    DOMUS_ASSERT(path.number_of_edges() > 0, "FaceSplitter::try_face_splits_with_path: empty path");
    DOMUS_ASSERT(
        m_embedding.get_degree_of_node(first_id) > 1,
        "FaceSplitter::try_face_splits_with_path: first node degree <= 1"
    );
    DOMUS_ASSERT(
        m_embedding.get_degree_of_node(last_id) > 1,
        "FaceSplitter::try_face_splits_with_path: last node degree <= 1"
    );

    insert_path_in_embedding(m_embedding, path);

    for (auto& combination : domus::utilities::generate_all_bitsets<2>()) {
        for (size_t i = 0; i < 2; i++)
            if (combination.test(i))
                m_embedding.reverse_circular_order(
                    path.node_id_at_position(i * path.number_of_edges())
                );
        std::vector<Path> paths = compute_faces_in_embedding(m_graph, m_embedding);
        std::optional<std::vector<Face>> faces = is_split_good(paths);
        if (faces.has_value())
            if (try_embedding_extension(*faces))
                return true;

        for (size_t i = 0; i < 2; i++)
            if (combination.test(i))
                m_embedding.reverse_circular_order(
                    path.node_id_at_position(i * path.number_of_edges())
                );
    }

    remove_path_from_embedding(m_embedding, path);
    return false;
}

bool SplitterWithPath::try_embedding_extension(const std::vector<Face>& faces) {
    DOMUS_ASSERT(
        compute_embedding_genus(m_embedding) == 1,
        "next_case: genus of embedding is not 1"
    );
    return handle_type_2(m_graph, m_embedding, faces);
}

bool handle_type_3(Graph& graph, Embedding& embedding, const Face& face, const size_t jolly_id) {
    const std::vector<Bridge> bridges = Bridge::compute(graph, embedding);

    SplitterWithPath splitter(graph, embedding, face, jolly_id, bridges);

    if (splitter.try_paths_inside_graph())
        return true;
    if (splitter.try_edges_not_in_graph())
        return true;

    return false;
}

} // namespace domus::torus
