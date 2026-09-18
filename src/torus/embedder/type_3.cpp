#include "type_3.hpp"

#include <ranges>

#include "domus/core/debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/core/print.hpp"
#include "domus/core/utils.hpp"

#include "../bridge.hpp"
#include "../faces.hpp"
#include "type_2.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

// this class models all the possible ways of splitting a type 3 face
class SplitterWithPath {
    enum class SplitOutcome { NO_LONGER_TOROIDAL, DOES_NOT_SPLIT, SPLITS };

    graph::Graph& m_graph;
    graph::Embedding& m_embedding;
    const Face& m_type_3_face;
    const size_t m_jolly_id;
    const std::vector<Bridge>& m_bridges;

    bool try_embedding_extension(const std::vector<Face>& faces);
    bool try_face_splits_with_path(const graph::Path& path);
    SplitOutcome is_split_good(const std::vector<Face>& faces);
    bool try_paths_inside_graph();
    bool try_edges_not_in_graph();

  public:
    SplitterWithPath(
        graph::Graph& graph,
        graph::Embedding& embedding,
        const Face& face,
        size_t jolly_id,
        const std::vector<Bridge>& bridges
    );
    bool try_all_possible_splits_and_keep_extending();
};

bool are_attachments_in_same_repeated_path(
    const NodesLabels<std::bitset<4>>& is_node_in_repeated_path,
    const size_t attachment_id_1,
    const size_t attachment_id_2
) {
    const auto& repeated_paths_1 = is_node_in_repeated_path.get_label(attachment_id_1);
    const auto& repeated_paths_2 = is_node_in_repeated_path.get_label(attachment_id_2);
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
        const size_t node_id_1 = path.get_node_id_at_position(i);
        const size_t node_id_2 = path.get_node_id_at_position(i + 1);
        const size_t edge_id = path.get_edge_id_at_position(i);
        embedding.add_edge(node_id_1, node_id_2, edge_id);
        embedding.add_edge(node_id_2, node_id_1, edge_id);
    }
}

void remove_path_from_embedding(Embedding& embedding, const Path& path) {
    for (size_t i = 0; i < path.number_of_edges(); ++i) {
        const size_t node_id_1 = path.get_node_id_at_position(i);
        const size_t node_id_2 = path.get_node_id_at_position(i + 1);
        const size_t edge_id = path.get_edge_id_at_position(i);
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
    const Face& type_3_face,
    size_t jolly_id,
    const std::vector<Bridge>& bridges
)
    : m_graph(graph), m_embedding(embedding), m_type_3_face(type_3_face), m_jolly_id(jolly_id),
      m_bridges(bridges) {}

auto candidate_face_splitting_paths_in_bridge(
    const Bridge& bridge,
    const NodesLabels<std::bitset<4>>& is_node_in_repeated_path,
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
    DOMUS_DEBUG_LN(
        "SplitterWithPath::try_edges_not_in_graph: splitting type 3 face with paths not in graph."
    );
    DOMUS_DEBUG_INDENT();
    for (size_t i = 0; i < m_type_3_face.path().number_of_nodes() - 1; i++) {
        const size_t node_id_1 = m_type_3_face.path().get_node_id_at_position(i);
        if (m_graph.get_degree_of_node(node_id_1) ==
            3) // cant add anything if the node is already cubic
            continue;
        for (size_t j = i + 1; j < m_type_3_face.path().number_of_nodes(); j++) {
            const size_t node_id_2 = m_type_3_face.path().get_node_id_at_position(j);
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
    DOMUS_DEBUG_LN(
        "SplitterWithPath::try_paths_inside_graph: splitting type 3 face with paths from bridges."
    );
    DOMUS_DEBUG_INDENT();
    for (const Bridge& bridge : m_bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(m_graph, bridge);
            if (try_face_splits_with_path(path))
                return true;
        } else {
            for (const Path& path : candidate_face_splitting_paths_in_bridge(
                     bridge,
                     m_type_3_face.is_node_in_repeated_path(),
                     m_graph
                 ))
                if (try_face_splits_with_path(path))
                    return true;
        }
    }
    return false;
}

SplitterWithPath::SplitOutcome SplitterWithPath::is_split_good(const std::vector<Face>& faces) {
    size_t isolated_nodes = 0;
    for (const size_t node_id : m_embedding.get_nodes_ids())
        if (m_embedding.get_degree_of_node(node_id) == 0)
            isolated_nodes++;
    if (compute_embedding_genus(
            m_embedding.get_number_of_nodes() - isolated_nodes,
            m_embedding.get_number_of_edges() / 2,
            faces.size(),
            1
        ) > 1)
        return SplitOutcome::NO_LONGER_TOROIDAL;
    for (const Face& face : faces)
        if (face.type() == FaceType::TYPE_3)
            return SplitOutcome::DOES_NOT_SPLIT;
    return SplitOutcome::SPLITS;
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
    DOMUS_DEBUG_LN(
        "SplitterWithPath::try_face_splits_with_path: trying split with {}",
        path.to_string()
    );

    insert_path_in_embedding(m_embedding, path);

    for (auto& combination : domus::utilities::generate_all_bitsets<2>()) {
        for (size_t i = 0; i < 2; i++)
            if (combination.test(i))
                m_embedding.reverse_circular_order(
                    path.get_node_id_at_position(i * path.number_of_edges())
                );

        std::vector<Path> face_paths = compute_faces_in_embedding(m_graph, m_embedding);
        std::vector<Face> faces;
        for (Path face_path : face_paths)
            faces.push_back(compute_face_from_path(std::move(face_path), m_graph));
        switch (is_split_good(faces)) {
        case SplitOutcome::SPLITS:
            DOMUS_DEBUG_LN("this path combination splits.");
            if (try_embedding_extension(faces))
                return true;
            break;
        case SplitOutcome::DOES_NOT_SPLIT:
            DOMUS_DEBUG_LN("this path combination does not split.");
            break;
        case SplitOutcome::NO_LONGER_TOROIDAL:
            DOMUS_DEBUG_LN("this path combination is not toroidal.");
            break;
        };

        for (size_t i = 0; i < 2; i++)
            if (combination.test(i))
                m_embedding.reverse_circular_order(
                    path.get_node_id_at_position(i * path.number_of_edges())
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
    DOMUS_DEBUG_INDENT();
    return handle_type_2(m_graph, m_embedding, faces);
}

bool SplitterWithPath::try_all_possible_splits_and_keep_extending() {
    DOMUS_DEBUG_LN("Trying to split face:");
    DOMUS_DEBUG("{}", m_type_3_face.to_string());
    DOMUS_DEBUG_LN("Bridges:");
    for (const Bridge& bridge : m_bridges)
        DOMUS_DEBUG("{}", bridge.to_string());
    if (try_paths_inside_graph()) // first we try to split with paths that are already inside the
                                  // graph
        return true;
    if (try_edges_not_in_graph()) // otherwise we then try paths that are not actually in the graph
                                  // ("ficticious paths")
        return true;
    return false; // if neither worked, no extension is possible
}

bool handle_type_3(Graph& graph, Embedding& embedding, const Face& face, const size_t jolly_id) {
    // at this point we want to insert some path in the embedding that splits the type 3 face. we
    // have two options to achieve this:
    // - 1: try paths from the bridges of the graphs
    // - 2: insert a path which is not in the graph in every possible way

    // the splitter class tries to insert these paths and extend the resulting "splitted" embedding
    // calling the procedure for type 2 faces next. if this pipeline eventually returns true, it
    // means we found an extension, otherwise no extension is possible

    const std::vector<Bridge> bridges = Bridge::compute(graph, embedding);
    SplitterWithPath splitter(graph, embedding, face, jolly_id, bridges);
    return splitter.try_all_possible_splits_and_keep_extending();
}

} // namespace domus::torus
