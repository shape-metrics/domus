#include "type_3.hpp"

#include "../splitter.hpp"
#include "../utils.hpp"

namespace domus::torus {
using namespace domus::graph;
using graph::utilities::NodesLabels;

class PathInsertionsCase3 {
    const Face& m_face;
    const NodesLabels<std::bitset<3>>& m_is_node_in_repeated_path;

    std::vector<PathInsertions>
    case_one_corner(const size_t first_edge_id, const size_t last_edge_id);
    std::vector<PathInsertions>
    case_one_inner(const size_t node_id, const size_t first_edge_id, const size_t last_edge_id);
    std::vector<PathInsertions> case_one_corner_one_inner(
        const size_t first_node_id,
        const size_t last_node_id,
        const size_t first_edge_id,
        const size_t last_edge_id,
        const size_t repeated_node_id
    );
    std::vector<PathInsertions> case_two_inner(
        const size_t first_node_id,
        const size_t last_node_id,
        const size_t first_edge_id,
        const size_t last_edge_id
    );

  public:
    PathInsertionsCase3(
        const Face& face, const NodesLabels<std::bitset<3>>& is_node_in_repeated_path
    );
    std::vector<PathInsertions> possible_insertions_of_path(const Path& path);
};

PathInsertionsCase3::PathInsertionsCase3(
    const Face& face, const NodesLabels<std::bitset<3>>& is_node_in_repeated_path
)
    : m_face(face), m_is_node_in_repeated_path(is_node_in_repeated_path) {}

std::vector<PathInsertions> PathInsertionsCase3::case_two_inner(
    const size_t first_node_id,
    const size_t last_node_id,
    const size_t first_edge_id,
    const size_t last_edge_id
) {
    std::vector<PathInsertions> insertions;

    const Path* repeated_path = nullptr;
    for (size_t i = 0; i < 2; i++)
        if (m_is_node_in_repeated_path.get_label(first_node_id).test(i)) {
            repeated_path = &m_face.repeated_paths()[i];
            DOMUS_ASSERT(
                m_is_node_in_repeated_path.get_label(last_node_id).test(i),
                "PathInsertionsCase3::case_two_inner: last node not in same repeated path"
            );
            break;
        }
    DOMUS_ASSERT(
        repeated_path != nullptr,
        "PathInsertionsCase3::case_two_inner: did not find repeated path with node"
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
        "PathInsertionsCase3::case_two_inner: did not find edge ids"
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

std::vector<PathInsertions> PathInsertionsCase3::case_one_corner_one_inner(
    const size_t first_node_id,
    const size_t last_node_id,
    const size_t first_edge_id,
    const size_t last_edge_id,
    const size_t repeated_node_id
) {
    std::vector<PathInsertions> insertions;

    const size_t internal_node_id =
        (first_node_id == repeated_node_id) ? last_node_id : first_node_id;
    const size_t edge_id_of_rep =
        (first_node_id == repeated_node_id) ? first_edge_id : last_edge_id;
    const size_t edge_id_of_inn =
        (first_node_id == repeated_node_id) ? last_edge_id : first_edge_id;

    const Path* repeated_path = nullptr;
    for (size_t i = 0; i < 2; i++)
        if (m_is_node_in_repeated_path.get_label(internal_node_id).test(i)) {
            repeated_path = &m_face.repeated_paths()[i];
            break;
        }
    DOMUS_ASSERT(
        repeated_path != nullptr,
        "PathInsertionsCase3::case_one_corner_one_inner: did not find repeated path with internal "
        "node"
    );

    std::optional<size_t> adjacent_edge_id;
    for (size_t i = 1; i < repeated_path->number_of_edges(); i++)
        if (repeated_path->node_id_at_position(i) == internal_node_id) {
            adjacent_edge_id = repeated_path->edge_id_at_position(i);
            break;
        }

    DOMUS_ASSERT(
        adjacent_edge_id.has_value(),
        "PathInsertionsCase3::case_one_corner_one_inner: did not find other edge id"
    );

    insertions.push_back(
        {{repeated_node_id,
          InsertionType::AFTER,
          repeated_path->get_first_edge_id(),
          edge_id_of_rep},
         {internal_node_id, InsertionType::BEFORE, *adjacent_edge_id, edge_id_of_inn}}
    );
    insertions.push_back(
        {{repeated_node_id,
          InsertionType::BEFORE,
          repeated_path->get_last_edge_id(),
          edge_id_of_rep},
         {internal_node_id, InsertionType::BEFORE, *adjacent_edge_id, edge_id_of_inn}}
    );

    insertions.push_back(
        {{repeated_node_id,
          InsertionType::BEFORE,
          repeated_path->get_first_edge_id(),
          edge_id_of_rep},
         {internal_node_id, InsertionType::AFTER, *adjacent_edge_id, edge_id_of_inn}}
    );
    insertions.push_back(
        {{repeated_node_id,
          InsertionType::AFTER,
          repeated_path->get_last_edge_id(),
          edge_id_of_rep},
         {internal_node_id, InsertionType::AFTER, *adjacent_edge_id, edge_id_of_inn}}
    );

    return insertions;
}

std::vector<PathInsertions> PathInsertionsCase3::case_one_inner(
    const size_t node_id, const size_t first_edge_id, const size_t last_edge_id
) {
    std::vector<PathInsertions> insertions;

    const Path* repeated_path = nullptr;
    for (size_t i = 0; i < 2; i++)
        if (m_is_node_in_repeated_path.get_label(node_id).test(i)) {
            repeated_path = &m_face.repeated_paths()[i];
            break;
        }
    DOMUS_ASSERT(
        repeated_path != nullptr,
        "PathInsertionsCase3::case_one_inner: did not find repeated path with internal "
        "node"
    );

    std::optional<size_t> adjacent_edge_id;
    for (size_t i = 1; i < repeated_path->number_of_edges(); i++)
        if (repeated_path->node_id_at_position(i) == node_id) {
            adjacent_edge_id = repeated_path->edge_id_at_position(i);
            break;
        }

    DOMUS_ASSERT(
        adjacent_edge_id.has_value(),
        "PathInsertionsCase3::case_one_inner: did not find other edge id"
    );

    insertions.push_back(
        {{node_id, InsertionType::AFTER, *adjacent_edge_id, first_edge_id},
         {node_id, InsertionType::BEFORE, *adjacent_edge_id, last_edge_id}}
    );
    insertions.push_back(
        {{node_id, InsertionType::BEFORE, *adjacent_edge_id, first_edge_id},
         {node_id, InsertionType::AFTER, *adjacent_edge_id, last_edge_id}}
    );

    return insertions;
}

std::vector<PathInsertions>
PathInsertionsCase3::case_one_corner(const size_t first_edge_id, const size_t last_edge_id) {
    std::vector<PathInsertions> insertions;
    const size_t node_id = m_face.repeated_paths()[0].get_first_node_id();

    for (const Path& repeated_path : m_face.repeated_paths()) {
        const size_t first_repeated_edge_id = repeated_path.get_first_edge_id();
        const size_t last_repeated_edge_id = repeated_path.get_last_edge_id();

        insertions.push_back(
            {{node_id, InsertionType::BEFORE, first_repeated_edge_id, first_edge_id},
             {node_id, InsertionType::AFTER, last_repeated_edge_id, last_edge_id}}
        );
        insertions.push_back(
            {{node_id, InsertionType::BEFORE, first_repeated_edge_id, last_edge_id},
             {node_id, InsertionType::AFTER, last_repeated_edge_id, first_edge_id}}
        );

        insertions.push_back(
            {{node_id, InsertionType::AFTER, first_repeated_edge_id, first_edge_id},
             {node_id, InsertionType::BEFORE, last_repeated_edge_id, last_edge_id}}
        );
        insertions.push_back(
            {{node_id, InsertionType::AFTER, first_repeated_edge_id, last_edge_id},
             {node_id, InsertionType::BEFORE, last_repeated_edge_id, first_edge_id}}
        );
    }

    const size_t first_repeated_edge_id = m_face.repeated_paths()[0].get_first_edge_id();
    const size_t last_repeated_edge_id = m_face.repeated_paths()[0].get_last_edge_id();
    insertions.push_back(
        {{node_id, InsertionType::AFTER, first_repeated_edge_id, first_edge_id},
         {node_id, InsertionType::AFTER, last_repeated_edge_id, last_edge_id}}
    );
    insertions.push_back(
        {{node_id, InsertionType::AFTER, first_repeated_edge_id, last_edge_id},
         {node_id, InsertionType::AFTER, last_repeated_edge_id, first_edge_id}}
    );

    insertions.push_back(
        {{node_id, InsertionType::BEFORE, first_repeated_edge_id, last_edge_id},
         {node_id, InsertionType::BEFORE, last_repeated_edge_id, first_edge_id}}
    );
    insertions.push_back(
        {{node_id, InsertionType::BEFORE, first_repeated_edge_id, first_edge_id},
         {node_id, InsertionType::BEFORE, last_repeated_edge_id, last_edge_id}}
    );

    return insertions;
}

std::vector<PathInsertions> PathInsertionsCase3::possible_insertions_of_path(const Path& path) {
    const size_t first_node_id = path.get_first_node_id();
    const size_t last_node_id = path.get_last_node_id();
    const size_t first_edge_id = path.get_first_edge_id();
    const size_t last_edge_id = path.get_last_edge_id();

    const size_t repeated_node_id = m_face.repeated_paths()[0].get_first_node_id();

    if (first_node_id == last_node_id) {
        if (first_node_id == repeated_node_id) {
            return case_one_corner(first_edge_id, last_edge_id);
        } else {
            return case_one_inner(first_node_id, first_edge_id, last_edge_id);
        }
    } else {
        if (first_node_id == repeated_node_id || last_node_id == repeated_node_id) {
            return case_one_corner_one_inner(
                first_node_id,
                last_node_id,
                first_edge_id,
                last_edge_id,
                repeated_node_id
            );
        } else {
            return case_two_inner(first_node_id, last_node_id, first_edge_id, last_edge_id);
        }
    }
}

bool handle_type_3(Graph& graph, Embedding& embedding, const Face& face, size_t jolly_id) {
    DOMUS_ASSERT(is_initial_face_valid(face), "handle_type_3: initial face is not valid");
    const std::vector<Bridge> bridges = Bridge::compute(graph, embedding);
    const NodesLabels<std::bitset<3>> is_node_in_repeated_path =
        compute_nodes_in_repeated_paths(graph, face);
    PathInsertionsCase3 paths_insertions_computer(face, is_node_in_repeated_path);

    SplitterWithPath splitter(
        graph,
        embedding,
        face,
        jolly_id,
        bridges,
        is_node_in_repeated_path,
        [&paths_insertions_computer](const Path& path) -> std::vector<PathInsertions> {
            return paths_insertions_computer.possible_insertions_of_path(path);
        }
    );

    if (splitter.try_paths_inside_graph())
        return true;
    if (splitter.try_edges_not_in_graph())
        return true;

    return false;
}

} // namespace domus::torus
