#include "splitter.hpp"

#include <ranges>

#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "../draw.hpp"
#include "utils.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

SplitterWithPath::SplitterWithPath(
    Graph& graph,
    Embedding& embedding,
    const Face& face,
    size_t jolly_id,
    const std::vector<Bridge>& bridges,
    const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    std::function<std::vector<PathInsertions>(const Path&)> compute_path_insertions
)
    : m_graph(graph), m_embedding(embedding), m_face(face), m_jolly_id(jolly_id),
      m_bridges(bridges), m_is_node_in_repeated_path(is_node_in_repeated_path),
      m_compute_path_insertions(compute_path_insertions) {}

auto candidate_face_splitting_paths_in_bridge(
    const Bridge& bridge,
    const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    const Graph& graph
) {
    return all_pairs_of_view(compute_all_feet_in_bridge(bridge)) |
           std::views::filter([&](const auto& pair) {
               const size_t from_id_1 = pair.first.edge.from_id;
               const size_t to_id_1 = pair.first.edge.to_id;
               const size_t attachment_1 = (bridge.is_attachment(from_id_1)) ? from_id_1 : to_id_1;
               const size_t from_id_2 = pair.second.edge.from_id;
               const size_t to_id_2 = pair.second.edge.to_id;
               const size_t attachment_2 = (bridge.is_attachment(from_id_2)) ? from_id_2 : to_id_2;

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
               const EdgeId& foot_1 = pair.first;
               const EdgeId& foot_2 = pair.second;
               const size_t from_id_1 = foot_1.edge.from_id;
               const size_t to_id_1 = foot_1.edge.to_id;
               const size_t attachment_1 = (bridge.is_attachment(from_id_1)) ? from_id_1 : to_id_1;
               const size_t inner_1 = (attachment_1 == from_id_1) ? to_id_1 : from_id_1;

               const size_t from_id_2 = foot_2.edge.from_id;
               const size_t to_id_2 = foot_2.edge.to_id;
               const size_t attachment_2 = (bridge.is_attachment(from_id_2)) ? from_id_2 : to_id_2;
               const size_t inner_2 = (attachment_2 == from_id_2) ? to_id_2 : from_id_2;

               Path path = (inner_1 != inner_2) ? algorithms::find_shortest_path_between_nodes(
                                                      bridge.get_bridge(),
                                                      inner_1,
                                                      inner_2
                                                  )
                                                      .value()
                                                : Path();
               path.push_front(bridge.get_bridge(), inner_1, foot_1.id);
               path.push_back(bridge.get_bridge(), inner_2, foot_2.id);
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

std::vector<size_t> SplitterWithPath::compute_attachments() {
    NodesContainer is_attachments_in_face(m_graph);
    std::vector<size_t> all_attachments;
    all_attachments.reserve(m_bridges.size() * 2);
    for (const Bridge& bridge : m_bridges) {
        const NodesLabels<size_t>& labels = bridge.get_new_id_to_old_id();
        for (const size_t attachment_id : bridge.get_attachments()) {
            const size_t old_id = labels.get_label(attachment_id);
            if (!is_attachments_in_face.has_node(old_id)) {
                is_attachments_in_face.add_node(old_id);
                all_attachments.push_back(old_id);
            }
        }
    }
    return all_attachments;
}

bool SplitterWithPath::try_edges_not_in_graph() {
    const std::vector<size_t> attachments = compute_attachments();

    for (size_t i = 0; i < attachments.size() - 1; i++) {
        const size_t node_id_1 = attachments[i];
        for (size_t j = i + 1; j < attachments.size(); j++) {
            const size_t node_id_2 = attachments[j];

            if (!are_attachments_in_same_repeated_path(
                    m_is_node_in_repeated_path,
                    node_id_1,
                    node_id_2
                )) {
                // In case we are in Case 4, this check is to skip Case 3.
                // In case we are in Case 3, this check is necessary to split the face.
                continue;
            }

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
                     m_is_node_in_repeated_path,
                     m_graph
                 ))
                if (try_face_splits_with_path(path))
                    return true;
        }
    }
    return false;
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

    augment_embedding_with_path(m_embedding, path);

    for (auto& [insertion_0, insertion_1] : m_compute_path_insertions(path)) {
        Edge e_0 = m_graph.get_edge(insertion_0.edge_id_to_insert);
        size_t from_id_0 = insertion_0.node_id;
        size_t to_id_0 = (e_0.from_id == from_id_0) ? e_0.to_id : e_0.from_id;

        if (insertion_0.type == InsertionType::AFTER)
            m_embedding.add_edge_after(
                from_id_0,
                to_id_0,
                insertion_0.edge_id_to_insert,
                insertion_0.edge_id
            );
        else
            m_embedding.add_edge_before(
                from_id_0,
                to_id_0,
                insertion_0.edge_id_to_insert,
                insertion_0.edge_id
            );

        Edge e_1 = m_graph.get_edge(insertion_1.edge_id_to_insert);
        size_t from_id_1 = insertion_1.node_id;
        size_t to_id_1 = (e_1.from_id == from_id_1) ? e_1.to_id : e_1.from_id;

        if (insertion_1.type == InsertionType::AFTER)
            m_embedding.add_edge_after(
                from_id_1,
                to_id_1,
                insertion_1.edge_id_to_insert,
                insertion_1.edge_id
            );
        else
            m_embedding.add_edge_before(
                from_id_1,
                to_id_1,
                insertion_1.edge_id_to_insert,
                insertion_1.edge_id
            );

        if (try_embedding_extension(path))
            return true;

        m_embedding.remove_edge(from_id_0, to_id_0, insertion_0.edge_id_to_insert);
        m_embedding.remove_edge(from_id_1, to_id_1, insertion_1.edge_id_to_insert);
    }

    remove_augment_of_path_in_embedding(m_embedding, path);
    return false;
}

bool SplitterWithPath::try_embedding_extension(const Path& path) {
    if (m_face.type() == FaceType::TYPE_4)
        draw_type_4_with_path(m_embedding, m_face, path);

    if (m_face.type() == FaceType::TYPE_3)
        draw_type_3_with_path(m_embedding, m_face, path);

    return next_case(m_embedding, m_graph, m_face);
}

} // namespace domus::torus