#include "3_stars.hpp"

#include <bitset>
#include <optional>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/torus/bridge.hpp"

#include "../../faces.hpp"
#include "../utils.hpp"

namespace domus::torus::stars {

using namespace domus::graph;
using utilities::NodesLabels;

class TrueThreeStar {
    const Bridge& m_bridge;
    const std::array<std::optional<size_t>, 3>& m_attachment_in_repeated_path;
    std::array<Path, 3> m_paths_to_center;

  public:
    TrueThreeStar(
        const Bridge& bridge,
        std::array<std::optional<size_t>, 3>& attachment_in_repeated_path,
        const Graph& graph
    );
    const std::array<Path, 3>& get_paths_to_center() const;
};

TrueThreeStar::TrueThreeStar(
    const Bridge& bridge,
    std::array<std::optional<size_t>, 3>& attachment_in_repeated_path,
    const Graph& graph
)
    : m_bridge(bridge), m_attachment_in_repeated_path(attachment_in_repeated_path) {
    // building 3-star from the bridge
    // find candidate center vertex (center of star)
    const size_t attachment_1 = *attachment_in_repeated_path[0];
    for (const auto edge : bridge.get_bridge().get_edges(attachment_1)) {
        const size_t old_attachment = bridge.get_new_id_to_old_id().get_label(attachment_1);
        const size_t old_edge_id = bridge.get_new_edge_id_to_old_id().get_label(edge.id);
        m_paths_to_center[0].push_back(graph, old_attachment, old_edge_id);
        break;
    }
    m_paths_to_center[1] = convert_path(
        algorithms::find_shortest_path_between_nodes(
            bridge.get_bridge(),
            *attachment_in_repeated_path[1],
            m_paths_to_center[0].get_last_node_id()
        )
            .value(),
        bridge.get_new_id_to_old_id(),
        bridge.get_new_edge_id_to_old_id(),
        graph
    );
    m_paths_to_center[2] = convert_path(
        algorithms::find_shortest_path_between_nodes(
            bridge.get_bridge(),
            *attachment_in_repeated_path[2],
            m_paths_to_center[0].get_last_node_id()
        )
            .value(),
        bridge.get_new_id_to_old_id(),
        bridge.get_new_edge_id_to_old_id(),
        graph
    );
    while (m_paths_to_center[1].get_last_edge_id() == m_paths_to_center[2].get_last_edge_id()) {
        m_paths_to_center[0].push_back(
            graph,
            m_paths_to_center[1].get_last_node_id(),
            m_paths_to_center[1].get_last_edge_id()
        );
        m_paths_to_center[1].pop_back();
        m_paths_to_center[2].pop_back();
    }
}

class FalseThreeStarBridge {
    Path m_path;

  public:
    FalseThreeStarBridge(
        const Bridge& bridge,
        const size_t attachment_id_1,
        const size_t attachment_id_2,
        const Graph& graph
    );
};

FalseThreeStarBridge::FalseThreeStarBridge(
    const Bridge& bridge,
    const size_t attachment_id_1,
    const size_t attachment_id_2,
    const Graph& graph
) {
    const Path path = algorithms::find_shortest_path_between_nodes(
                          bridge.get_bridge(),
                          attachment_id_1,
                          attachment_id_2
    )
                          .value();
    m_path = convert_path(
        path,
        bridge.get_new_id_to_old_id(),
        bridge.get_new_edge_id_to_old_id(),
        graph
    );
}

class ThreeStarsHandler {
    const Face& m_face;
    const NodesLabels<std::bitset<3>>& m_is_node_in_repeated_path;
    Graph& m_graph;
    Embedding& m_embedding;
    std::vector<TrueThreeStar> m_candidate_true_3_stars;
    std::vector<std::optional<std::array<size_t, 3>>>
        m_spreaded_true_3_stars_center_circular_order_of_edge_ids;
    std::vector<FalseThreeStarBridge> m_bridges_0_1;
    std::vector<FalseThreeStarBridge> m_bridges_0_2;
    std::vector<FalseThreeStarBridge> m_bridges_1_2;
    bool case_true_star();
    void insert_spreaded_true_star(const TrueThreeStar& star);
    bool is_circular_order_of_center_spreaded(size_t star_index);
    void remove_true_star(const TrueThreeStar& star);
    void populate_spreaded_circular_order(size_t star_index);
    bool case_false_star();
    bool case_zero_3_stars();
    bool case_one_3_star();
    bool case_two_or_three_3_stars();

  public:
    ThreeStarsHandler(
        const Face& face,
        const std::vector<Bridge>& bridges,
        const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
        Graph& graph,
        Embedding& embedding
    );
    bool solve();
};

ThreeStarsHandler::ThreeStarsHandler(
    const Face& face,
    const std::vector<Bridge>& bridges,
    const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    Graph& graph,
    Embedding& embedding
)
    : m_face(face), m_is_node_in_repeated_path(is_node_in_repeated_path), m_graph(graph),
      m_embedding(embedding) {
    const size_t first_node_id = face.repeated_paths()[0].get_first_node_id();
    const size_t last_node_id = face.repeated_paths()[0].get_last_node_id();
    for (const Bridge& bridge : bridges) {
        std::array<std::optional<size_t>, 3> attachment_in_repeated_path{
            std::nullopt,
            std::nullopt,
            std::nullopt
        };
        for (const size_t attachment_id : bridge.get_attachments()) {
            const size_t old_attachment_id = bridge.get_new_id_to_old_id().get_label(attachment_id);
            if (old_attachment_id == first_node_id || old_attachment_id == last_node_id)
                continue;
            const std::bitset<3>& is_in_repeated_path =
                is_node_in_repeated_path.get_label(old_attachment_id);
            for (size_t i = 0; i < 3; i++)
                if (is_in_repeated_path.test(i))
                    attachment_in_repeated_path[i] = attachment_id;
        }
        if (attachment_in_repeated_path[0].has_value() &&
            attachment_in_repeated_path[1].has_value() &&
            attachment_in_repeated_path[2].has_value()) {
            m_candidate_true_3_stars.push_back(
                TrueThreeStar(bridge, attachment_in_repeated_path, graph)
            );
            continue;
        }
        if (attachment_in_repeated_path[0].has_value() &&
            attachment_in_repeated_path[1].has_value()) {
            m_bridges_0_1.push_back(FalseThreeStarBridge(
                bridge,
                *attachment_in_repeated_path[0],
                *attachment_in_repeated_path[1],
                graph
            ));
            continue;
        }
        if (attachment_in_repeated_path[0].has_value() &&
            attachment_in_repeated_path[2].has_value()) {
            m_bridges_0_2.push_back(FalseThreeStarBridge(
                bridge,
                *attachment_in_repeated_path[0],
                *attachment_in_repeated_path[2],
                graph
            ));
            continue;
        }
        if (attachment_in_repeated_path[1].has_value() &&
            attachment_in_repeated_path[2].has_value())
            m_bridges_1_2.push_back(FalseThreeStarBridge(
                bridge,
                *attachment_in_repeated_path[1],
                *attachment_in_repeated_path[2],
                graph
            ));
    }
    m_spreaded_true_3_stars_center_circular_order_of_edge_ids.reserve(
        m_candidate_true_3_stars.size()
    );
    for (size_t i = 0; i < m_candidate_true_3_stars.size(); ++i)
        m_spreaded_true_3_stars_center_circular_order_of_edge_ids.push_back(std::nullopt);
}

/**
 * @brief Embeds the true star in the face, such that it is "spreaded". The circular order of the
 * legs of the star is guaranteed to be good (it corresponds to one of the two admissible embeddings
 * of a spreaded true star, we do not care which one). However that is not the case for the circular
 * order of the center. Assuming this is the only true star inserted in the face, you need to later
 * check if the resulting embedding keeps being toroidal (in such case it is good), otherwise it
 * needs to be reversed.
 *
 * @param star The star to be inserted in the face.
 */
void ThreeStarsHandler::insert_spreaded_true_star(const TrueThreeStar& star) {
    std::array<size_t, 3> repeated_path_index_of_first_node{3, 3, 3};
    for (size_t i = 0; i < 2; ++i) {
        const Path& path_to_center = star.get_paths_to_center()[i];
        DOMUS_ASSERT(
            (m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(0) +
             m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(1) +
             m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(2)) == 1,
            "ThreeStarsHandler::insert_spreaded_true_star: initial node should be in exactly one "
            "repeated path"
        );
        if (m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(0))
            repeated_path_index_of_first_node[0] = 0;
        if (m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(1))
            repeated_path_index_of_first_node[0] = 1;
        if (m_is_node_in_repeated_path.get_label(path_to_center.get_first_node_id()).test(2))
            repeated_path_index_of_first_node[0] = 2;
    }
    DOMUS_ASSERT(
        [&]() {
            if (repeated_path_index_of_first_node[0] == 3)
                return false;
            if (repeated_path_index_of_first_node[1] == 3)
                return false;
            if (repeated_path_index_of_first_node[2] == 3)
                return false;
            return repeated_path_index_of_first_node[0] != repeated_path_index_of_first_node[1] &&
                   repeated_path_index_of_first_node[0] != repeated_path_index_of_first_node[2] &&
                   repeated_path_index_of_first_node[2] != repeated_path_index_of_first_node[1];
        }(),
        "ThreeStarsHandler::insert_spreaded_true_star: found indexes are not good"
    );
    for (size_t i = 0; i < 2; ++i) {
        const Path& path_to_center = star.get_paths_to_center()[i];
        const Path& adjacent_repeated_path =
            m_face.repeated_paths()[repeated_path_index_of_first_node[i]];
        for (size_t j = 1; j < path_to_center.number_of_edges(); ++j) {
            const size_t node_id_1 = path_to_center.node_id_at_position(j);
            const size_t node_id_2 = path_to_center.node_id_at_position(j + 1);
            const size_t edge_id = path_to_center.edge_id_at_position(j);
            m_embedding.add_edge(node_id_1, node_id_2, edge_id);
            m_embedding.add_edge(node_id_2, node_id_1, edge_id);
        }
        for (size_t j = 1; j < adjacent_repeated_path.number_of_nodes() - 1; ++j) {
            const size_t node_id_1 = path_to_center.node_id_at_position(j);
            if (node_id_1 != path_to_center.get_first_node_id())
                continue;
            const size_t node_id_2 = path_to_center.node_id_at_position(1);
            const size_t edge_id = path_to_center.edge_id_at_position(0);
            m_embedding.add_edge_after(
                node_id_1,
                node_id_2,
                edge_id,
                adjacent_repeated_path.edge_id_at_position(j)
            );
            m_embedding.add_edge(node_id_2, node_id_1, edge_id);
            break;
        }
    }
}

void ThreeStarsHandler::remove_true_star(const TrueThreeStar& star) {
    for (size_t i = 0; i < 2; ++i) {
        const Path& path_to_center = star.get_paths_to_center()[i];
        for (size_t j = 0; j < path_to_center.number_of_edges(); ++j) {
            const size_t node_id_1 = path_to_center.node_id_at_position(j);
            const size_t node_id_2 = path_to_center.node_id_at_position(j + 1);
            const size_t edge_id = path_to_center.edge_id_at_position(j);
            m_embedding.remove_edge(node_id_1, node_id_2, edge_id);
            m_embedding.remove_edge(node_id_2, node_id_1, edge_id);
        }
    }
}

/**
 * @brief Each true star has two ways to be embedded "spreaded", however the circular order of the
 * center of the star is the same in both cases. Conversely, in case the true star is embedded
 * narrow (6 different embeddings), in all of them the circular order is the opposite of the one
 * when embedded spreaded.
 * This function computes the corresponding circular order when the star is embedded spreaded (and
 * is assumed to be so in m_embedding, other than being the only star currently embedded in the
 * face). This is usefull to remember when we want to compute the possible "narrow" embeddings.
 * @param star_index Index of the star in m_candidate_true_3_stars array
 */
void ThreeStarsHandler::populate_spreaded_circular_order(size_t star_index) {
    const TrueThreeStar& star = m_candidate_true_3_stars[star_index];
    const size_t center_node_id = star.get_paths_to_center()[0].get_last_node_id();
    DOMUS_ASSERT(
        m_embedding.get_degree_of_node(center_node_id) == 3,
        "ThreeStarsHandler::populate_spreaded_circular_order: center of star should have degree 3"
    );
    DOMUS_ASSERT(
        !m_spreaded_true_3_stars_center_circular_order_of_edge_ids[star_index].has_value(),
        "ThreeStarsHandler::populate_spreaded_circular_order: circular order already initialized"
    );
    m_spreaded_true_3_stars_center_circular_order_of_edge_ids[star_index] = {0, 0, 0};
    std::array<size_t, 3>& circular_order =
        m_spreaded_true_3_stars_center_circular_order_of_edge_ids[star_index].value();
    size_t i = 0;
    for (const EdgeIter edge : m_embedding.get_edges(center_node_id)) {
        circular_order[i] = edge.id;
        ++i;
    }
}

bool ThreeStarsHandler::case_true_star() {
    for (size_t i = 0; i < m_candidate_true_3_stars.size(); ++i) {
        const TrueThreeStar& candidate_3_star = m_candidate_true_3_stars[i];
        insert_spreaded_true_star(candidate_3_star);
        // Check if center of star had bad circular order
        if (compute_embedding_genus(m_embedding) > 1) {
            const size_t center_id = candidate_3_star.get_paths_to_center()[0].get_last_node_id();
            m_embedding.reverse_circular_order(center_id);
        }
        populate_spreaded_circular_order(i);
        if (next_case(m_embedding, m_graph))
            return true;
        /* A spreaded true star admits two embeddings, both with the same circular order of the
           center. What differs are the circular order of the legs of the star. Then, to obtain one
           embedding from the other, you just need to reverse all the circular order of the
           legs. */
        for (const Path& path : candidate_3_star.get_paths_to_center())
            m_embedding.reverse_circular_order(path.get_first_node_id());
        if (next_case(m_embedding, m_graph))
            return true;
        remove_true_star(candidate_3_star);
    }
    return false;
}

bool ThreeStarsHandler::case_false_star() {
    if (m_candidate_true_3_stars.size() == 4) // then one must be spreaded
        return false;

    // Checking if a false star can be built at all
    if (m_bridges_0_1.size() == 0)
        return false;
    if (m_bridges_1_2.size() == 0)
        return false;
    if (m_bridges_0_2.size() == 0)
        return false;

    // Trying false stars
    if (m_candidate_true_3_stars.size() == 0)
        return case_zero_3_stars();
    if (m_candidate_true_3_stars.size() == 1)
        return case_one_3_star();
    return case_two_or_three_3_stars();
}

bool ThreeStarsHandler::case_zero_3_stars() {
    // TODO find the two smallest of the three bridges sets and use those

    // TODO
}

bool ThreeStarsHandler::case_one_3_star() {
    // TODO
}

// TODO can probably be made more efficient in discarding no-go cases
bool ThreeStarsHandler::case_two_or_three_3_stars() {
    const size_t k = m_candidate_true_3_stars.size();
    DOMUS_ASSERT(
        k == 2 || k == 3,
        "ThreeStarsHandler::case_two_or_three_3_stars: function only for the case of two or three "
        "3-stars"
    );
    for (size_t i = 0; i < k; ++i) {
        if (is_circular_order_of_center_spreaded(i)) {
            const size_t center_id =
                m_candidate_true_3_stars[i].get_paths_to_center()[0].get_last_node_id();
            m_embedding.reverse_circular_order(center_id);
        }
    }

    constexpr std::array<std::array<bool, 3>, 6> configurations{
        {{true, true, false},
         {true, false, true},
         {false, true, true},
         {true, false, false},
         {false, false, true},
         {false, true, false}}
    };

    auto setup_circular_order_of_legs = [&](const std::array<bool, 3>& configuration,
                                            const TrueThreeStar& star) {
        for (size_t i = 0; i < 2; ++i)
            if (configuration[i])
                m_embedding.reverse_circular_order(
                    star.get_paths_to_center()[i].get_first_node_id()
                );
    };

    auto explore_configurations = [&](auto& self, size_t star_idx) -> bool {
        // Base case: all 'k' stars have been configured
        if (star_idx == k) {
            if (compute_embedding_genus(m_embedding) == 1) // TODO check how many times happens
                if (next_case(m_embedding, m_graph))
                    return true;
            return false;
        }

        // Recursive case: try all configurations for the current star
        const TrueThreeStar& current_star = m_candidate_true_3_stars[star_idx];
        for (const auto& config : configurations) {
            setup_circular_order_of_legs(config, current_star);
            // Recurse to the next star
            if (self(self, star_idx + 1))
                return true;
            setup_circular_order_of_legs(config, current_star);
        }

        return false;
    };

    return explore_configurations(explore_configurations, 0);
}

bool ThreeStarsHandler::solve() {
    if (m_candidate_true_3_stars.size() > 4)
        return false;
    if (case_true_star())
        return true;
    if (case_false_star())
        return true;
    return false;
}

bool try_3_stars(
    const Face& face,
    const std::vector<Bridge>& bridges,
    const NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    Graph& graph,
    Embedding& embedding
) {
    ThreeStarsHandler handler(face, bridges, is_node_in_repeated_path, graph, embedding);
    return handler.solve();
}

} // namespace domus::torus::stars
