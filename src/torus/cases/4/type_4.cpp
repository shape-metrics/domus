#include "type_4.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/torus/bridge.hpp"

#include "../../faces.hpp"
#include "../utils.hpp"
#include "3_stars.hpp"

namespace domus::torus {
using namespace domus::graph;
using graph::utilities::NodesContainer;
using graph::utilities::NodesLabels;

enum class InsertionType { AFTER, BEFORE };

struct Insertion {
    const size_t node_id;
    const InsertionType type;
    const size_t edge_id;
    const size_t edge_id_to_insert;
};

class Type4Handler {
    Graph& m_graph;
    Embedding& m_embedding;
    const Face& m_face;
    const size_t m_jolly_id;
    std::vector<Bridge> m_bridges;
    NodesLabels<std::bitset<3>> m_is_node_in_repeated_path;

    bool try_paths_inside_graph();
    bool try_edges_not_in_graph();
    bool try_3_stars();
    bool try_face_splits_with_path(const Path& path);
    std::vector<std::pair<Insertion, Insertion>>
    case_path_is_loop(const size_t first_id, const size_t first_edge_id, const size_t last_edge_id);
    std::vector<std::pair<Insertion, Insertion>> case_two_corners_of_hexagon(
        const size_t first_node_id,
        const size_t last_node_id,
        const size_t first_edge_id,
        const size_t last_edge_id
    );
    std::vector<std::pair<Insertion, Insertion>> case_one_corner_of_hexagon(
        const size_t first_node_id,
        const size_t last_node_id,
        const size_t first_edge_id,
        const size_t last_edge_id,
        const size_t first_repeated_node_id,
        const size_t last_repeated_node_id
    );
    std::vector<std::pair<Insertion, Insertion>> case_no_corner_of_hexagon(
        const size_t first_node_id,
        const size_t last_node_id,
        const size_t first_edge_id,
        const size_t last_edge_id
    );
    std::vector<std::pair<Insertion, Insertion>> possible_insertions_of_path(const Path& path);
    auto candidate_face_splitting_paths_in_bridge(const Bridge& bridge);

  public:
    Type4Handler(Graph& graph, Embedding& embedding, const Face& face, size_t jolly_id);
    bool solve();
};

std::vector<std::pair<Insertion, Insertion>> Type4Handler::case_path_is_loop(
    const size_t first_node_id, const size_t first_edge_id, const size_t last_edge_id
) {
    std::vector<std::pair<Insertion, Insertion>> insertions;

    std::vector<size_t> edge_ids;
    edge_ids.reserve(3);
    for (EdgeIter edge_it : m_embedding.get_edges(first_node_id))
        edge_ids.push_back(edge_it.id);

    DOMUS_ASSERT(
        edge_ids.size() == 3,
        "Type4Handler::case_path_is_loop: expected to find 3 neighbors"
    );

    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[0], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[1], last_edge_id}}
    );
    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[1], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[0], last_edge_id}}
    );

    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[0], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[2], last_edge_id}}
    );
    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[2], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[0], last_edge_id}}
    );

    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[1], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[2], last_edge_id}}
    );
    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[2], first_edge_id},
         {first_node_id, InsertionType::AFTER, edge_ids[1], last_edge_id}}
    );

    return insertions;
}

std::vector<std::pair<Insertion, Insertion>> Type4Handler::case_two_corners_of_hexagon(
    const size_t first_node_id,
    const size_t last_node_id,
    const size_t first_edge_id,
    const size_t last_edge_id
) {
    std::vector<std::pair<Insertion, Insertion>> insertions;

    if (m_face.repeated_paths()[0].get_first_node_id() == first_node_id) {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[0].get_first_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[0].get_last_edge_id(),
              last_edge_id}}
        );
    } else {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[0].get_last_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[0].get_first_edge_id(),
              last_edge_id}}
        );
    }

    if (m_face.repeated_paths()[1].get_first_node_id() == first_node_id) {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[1].get_first_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[1].get_last_edge_id(),
              last_edge_id}}
        );
    } else {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[1].get_last_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[1].get_first_edge_id(),
              last_edge_id}}
        );
    }

    if (m_face.repeated_paths()[2].get_first_node_id() == first_node_id) {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[2].get_first_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[2].get_last_edge_id(),
              last_edge_id}}
        );
    } else {
        insertions.push_back(
            {{first_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[2].get_last_edge_id(),
              first_edge_id},
             {last_node_id,
              InsertionType::AFTER,
              m_face.repeated_paths()[2].get_first_edge_id(),
              last_edge_id}}
        );
    }

    return insertions;
}

std::vector<std::pair<Insertion, Insertion>> Type4Handler::case_one_corner_of_hexagon(
    const size_t first_node_id,
    const size_t last_node_id,
    const size_t first_edge_id,
    const size_t last_edge_id,
    const size_t first_repeated_node_id,
    const size_t last_repeated_node_id
) {
    std::vector<std::pair<Insertion, Insertion>> insertions;

    const size_t repeated_node_id =
        (first_node_id == first_repeated_node_id || first_node_id == last_repeated_node_id)
            ? first_node_id
            : last_node_id;
    const size_t internal_node_id =
        (first_node_id == first_repeated_node_id || first_node_id == last_repeated_node_id)
            ? last_node_id
            : first_node_id;
    const size_t repeated_edge_id =
        (first_node_id == repeated_node_id) ? first_edge_id : last_edge_id;
    const size_t internal_edge_id =
        (first_node_id == repeated_node_id) ? last_edge_id : first_edge_id;

    const Path* repeated_path = nullptr;
    for (size_t i = 0; i < 3; i++)
        if (m_is_node_in_repeated_path.get_label(internal_node_id).test(i)) {
            repeated_path = &m_face.repeated_paths()[i];
            break;
        }
    DOMUS_ASSERT(
        repeated_path != nullptr,
        "Type4Handler::case_one_corner_of_hexagon: did not find repeated path with internal node"
    );

    const size_t edge_id = (repeated_path->get_first_node_id() == repeated_node_id)
                               ? repeated_path->get_first_edge_id()
                               : repeated_path->get_last_edge_id();
    std::optional<size_t> other_edge_id;
    if (repeated_path->get_first_node_id() == repeated_node_id) {
        for (size_t i = 1; i < repeated_path->number_of_edges(); i++)
            if (repeated_path->node_id_at_position(i) == internal_node_id) {
                other_edge_id = repeated_path->edge_id_at_position(i);
                break;
            }
    } else {
        for (size_t i = 1; i < repeated_path->number_of_edges(); i++)
            if (repeated_path->node_id_at_position(i) == internal_node_id) {
                other_edge_id = repeated_path->edge_id_at_position(i - 1);
                break;
            }
    }

    DOMUS_ASSERT(
        other_edge_id.has_value(),
        "Type4Handler::case_one_corner_of_hexagon: did not find other edge id"
    );

    insertions.push_back(
        {{repeated_node_id, InsertionType::AFTER, edge_id, repeated_edge_id},
         {internal_node_id, InsertionType::BEFORE, other_edge_id.value(), internal_edge_id}}
    );
    insertions.push_back(
        {{repeated_node_id, InsertionType::BEFORE, edge_id, repeated_edge_id},
         {internal_node_id, InsertionType::AFTER, other_edge_id.value(), internal_edge_id}}
    );

    return insertions;
}

std::vector<std::pair<Insertion, Insertion>> Type4Handler::case_no_corner_of_hexagon(
    const size_t first_node_id,
    const size_t last_node_id,
    const size_t first_edge_id,
    const size_t last_edge_id
) {
    std::vector<std::pair<Insertion, Insertion>> insertions;

    const Path* repeated_path = nullptr;
    for (size_t i = 0; i < 3; i++)
        if (m_is_node_in_repeated_path.get_label(first_node_id).test(i)) {
            repeated_path = &m_face.repeated_paths()[i];
            DOMUS_ASSERT(
                m_is_node_in_repeated_path.get_label(last_node_id).test(i),
                "Type4Handler::case_no_corner_of_hexagon: last node not in same repeated path"
            );
            break;
        }
    DOMUS_ASSERT(
        repeated_path != nullptr,
        "Type4Handler::case_no_corner_of_hexagon: did not find repeated path with node"
    );

    std::vector<std::optional<size_t>> edge_ids(2, std::nullopt);
    for (size_t i = 1; i < repeated_path->number_of_edges(); i++) {
        if (repeated_path->node_id_at_position(i) == first_node_id)
            edge_ids[0] = repeated_path->edge_id_at_position(i);
        if (repeated_path->node_id_at_position(i) == last_node_id)
            edge_ids[1] = repeated_path->edge_id_at_position(i);
    }

    DOMUS_ASSERT(
        edge_ids[0].has_value() && edge_ids[1].has_value(),
        "Type4Handler::case_no_corner_of_hexagon: did not find edge ids"
    );

    insertions.push_back(
        {{first_node_id, InsertionType::AFTER, edge_ids[0].value(), first_edge_id},
         {last_node_id, InsertionType::BEFORE, edge_ids[1].value(), last_edge_id}}
    );
    insertions.push_back(
        {{first_node_id, InsertionType::BEFORE, edge_ids[0].value(), first_edge_id},
         {last_node_id, InsertionType::AFTER, edge_ids[1].value(), last_edge_id}}
    );

    return insertions;
}

std::vector<std::pair<Insertion, Insertion>>
Type4Handler::possible_insertions_of_path(const Path& path) {
    const size_t first_node_id = path.get_first_node_id();
    const size_t last_node_id = path.get_last_node_id();
    const size_t first_edge_id = path.get_first_edge_id();
    const size_t last_edge_id = path.get_last_edge_id();

    const size_t first_repeated_node_id = m_face.repeated_paths()[0].get_first_node_id();
    const size_t last_repeated_node_id = m_face.repeated_paths()[0].get_last_node_id();

    if ((first_node_id == first_repeated_node_id && last_node_id == last_repeated_node_id) ||
        (first_node_id == last_repeated_node_id && last_node_id == first_repeated_node_id)) {
        if (first_node_id == last_node_id)
            return case_path_is_loop(first_node_id, first_edge_id, last_edge_id);
        else
            return case_two_corners_of_hexagon(
                first_node_id,
                last_node_id,
                first_edge_id,
                last_edge_id
            );
    } else if (
        first_node_id == first_repeated_node_id || first_node_id == last_repeated_node_id ||
        last_node_id == last_repeated_node_id || last_node_id == first_repeated_node_id
    ) {
        return case_one_corner_of_hexagon(
            first_node_id,
            last_node_id,
            first_edge_id,
            last_edge_id,
            first_repeated_node_id,
            last_repeated_node_id
        );
    } else {
        return case_no_corner_of_hexagon(first_node_id, last_node_id, first_edge_id, last_edge_id);
    }
}

auto Type4Handler::candidate_face_splitting_paths_in_bridge(const Bridge& bridge) {
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
                   m_is_node_in_repeated_path,
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

               Path path = (inner_1 != inner_2)
                               ? graph::algorithms::find_shortest_path_between_nodes(
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
                   old_path.push_back(m_graph, old_prev_node_id, old_edge_id);
               }
               return old_path;
           });
}

bool Type4Handler::try_face_splits_with_path(const Path& path) {
    DOMUS_ASSERT(
        compute_embedding_genus(m_embedding) == 1,
        "Type4Handler::try_face_splits_with_path: initial genus of embedding is not 1"
    );
    const size_t first_id = path.get_first_node_id();
    const size_t last_id = path.get_last_node_id();

    DOMUS_ASSERT(path.number_of_edges() > 0, "Type4Handler::try_face_splits_with_path: empty path");
    DOMUS_ASSERT(
        m_embedding.get_degree_of_node(first_id) > 1,
        "Type4Handler::try_face_splits_with_path: first node degree <= 1"
    );
    DOMUS_ASSERT(
        m_embedding.get_degree_of_node(last_id) > 1,
        "Type4Handler::try_face_splits_with_path: last node degree <= 1"
    );

    augment_embedding_with_path(m_embedding, path);

    for (auto& [insertion_0, insertion_1] : possible_insertions_of_path(path)) {
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

        if (next_case(m_embedding, m_graph))
            return true;

        m_embedding.remove_edge(from_id_0, to_id_0, insertion_0.edge_id_to_insert);
        m_embedding.remove_edge(from_id_1, to_id_1, insertion_1.edge_id_to_insert);
    }

    remove_augment_of_path_in_embedding(m_embedding, path);
    return false;
}

bool Type4Handler::try_paths_inside_graph() {
    for (const Bridge& bridge : m_bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(m_graph, bridge);
            const size_t attachment_id_1 = path.get_first_node_id();
            const size_t attachment_id_2 = path.get_last_node_id();
            if (are_attachments_in_same_repeated_path(
                    m_is_node_in_repeated_path,
                    attachment_id_1,
                    attachment_id_2
                ))
                if (try_face_splits_with_path(path))
                    return true;
        } else {
            for (const Path& path : candidate_face_splitting_paths_in_bridge(bridge))
                if (try_face_splits_with_path(path))
                    return true;
        }
    }
    return false;
}

bool Type4Handler::try_edges_not_in_graph() {
    NodesContainer is_attachments_in_face(m_graph);
    std::vector<size_t> attachments_in_face;
    attachments_in_face.reserve(m_bridges.size() * 2);
    for (const Bridge& bridge : m_bridges) {
        const NodesLabels<size_t>& labels = bridge.get_new_id_to_old_id();
        for (const size_t attachment_id : bridge.get_attachments()) {
            const size_t old_id = labels.get_label(attachment_id);
            if (!is_attachments_in_face.has_node(old_id)) {
                is_attachments_in_face.add_node(old_id);
                attachments_in_face.push_back(old_id);
            }
        }
    }

    for (size_t i = 0; i < attachments_in_face.size() - 1; i++) {
        const size_t node_id_1 = attachments_in_face[i];
        for (size_t j = i + 1; j < attachments_in_face.size(); j++) {
            const size_t node_id_2 = attachments_in_face[j];

            if (!are_attachments_in_same_repeated_path(
                    m_is_node_in_repeated_path,
                    node_id_1,
                    node_id_2
                ))
                continue;

            DOMUS_ASSERT(
                m_graph.get_degree_of_node(m_jolly_id) == 0,
                "Type4Handler::try_edges_not_in_graph: no jolly node available"
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

bool Type4Handler::try_3_stars() {
    return stars::try_3_stars(m_face, m_bridges, m_is_node_in_repeated_path, m_graph, m_embedding);
}

bool Type4Handler::solve() {
    if (try_paths_inside_graph())
        return true;
    if (try_edges_not_in_graph())
        return true;
    if (try_3_stars())
        return true;
    return false;
}

Type4Handler::Type4Handler(Graph& graph, Embedding& embedding, const Face& face, size_t jolly_id)
    : m_graph(graph), m_embedding(embedding), m_face(face), m_jolly_id(jolly_id),
      m_is_node_in_repeated_path(graph) {
    m_bridges = Bridge::compute(graph, embedding);

    for (const size_t node_id : graph.get_nodes_ids())
        m_is_node_in_repeated_path.add_label(node_id, {});
    for (size_t i = 0; i < face.repeated_paths().size(); i++) {
        const Path& repeated_path = face.repeated_paths()[i];
        for (size_t j = 1; j < repeated_path.number_of_nodes() - 1; j++) {
            const size_t node_id = repeated_path.node_id_at_position(j);
            m_is_node_in_repeated_path.get_label(node_id).set(i);
        }
    }
    const size_t first_id = face.repeated_paths()[0].get_first_node_id();
    const size_t last_id = face.repeated_paths()[0].get_last_node_id();
    m_is_node_in_repeated_path.get_label(first_id).set();
    m_is_node_in_repeated_path.get_label(last_id).set();
}

bool handle_type_4(Graph& graph, Embedding& embedding, const Face& face, size_t jolly_id) {
    Type4Handler handler(graph, embedding, face, jolly_id);
    return handler.solve();
}

} // namespace domus::torus
