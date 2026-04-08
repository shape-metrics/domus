#include "next_type_establisher.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/path.hpp"
#include <print>

namespace domus::torus {
using graph::Graph;
using graph::Path;

NextTypesEstablisher::NextTypesEstablisher(
    const graph::Graph& graph,
    const graph::Path& splitting_path,
    const Face& old_type_4_face,
    const graph::Path& face_1,
    const graph::Path& face_2
)
    : m_graph(graph), m_splitting_path(splitting_path), m_old_type_4_face(old_type_4_face),
      m_face_1(face_1), m_face_2(face_2) {}

std::vector<Path>
NextTypesEstablisher::repeated_paths_both_common_endpoints(const Path& type_1_face) {
    const size_t first_node_id = m_splitting_path.get_first_node_id();
    std::optional<size_t> pos;
    for (size_t i = 0; i < type_1_face.number_of_edges(); ++i)
        if (type_1_face.node_id_at_position(i) == first_node_id) {
            pos = i;
            break;
        }

    DOMUS_ASSERT(
        pos.has_value(),
        "repeated_paths_both_common_endpoints: did not find expected node id"
    );

    const size_t edge_id = // an edge_id that was in a repeating path before, but will no longer be
        (type_1_face.node_id_at_position(*pos + 1) == m_splitting_path.node_id_at_position(1))
            ? type_1_face.edge_id_at_position(
                  (*pos + type_1_face.number_of_edges() - 1) % type_1_face.number_of_edges()
              )
            : type_1_face.edge_id_at_position(*pos);

    std::vector<Path> repeated_paths;
    for (const Path& old_repeated_path : m_old_type_4_face.repeated_paths()) {
        if (old_repeated_path.get_first_edge_id() == edge_id ||
            old_repeated_path.get_last_edge_id() == edge_id)
            continue;
        repeated_paths.push_back(old_repeated_path);
    }
    DOMUS_ASSERT(
        repeated_paths.size() == 2,
        "repeated_paths_both_common_endpoints: should have obtained 2 repeated paths"
    );
    return repeated_paths;
}

Path NextTypesEstablisher::repeated_path_one_common_endpoint_loop(const Path& type_1_face) {
    const size_t first_node_id = m_splitting_path.get_first_node_id();
    std::array<size_t, 2> pos;
    size_t current = 0;
    for (size_t i = 0; i < type_1_face.number_of_edges(); ++i)
        if (type_1_face.node_id_at_position(i) == first_node_id)
            pos[current++] = i;

    DOMUS_ASSERT(
        current == 2,
        "repeated_path_one_common_endpoint_loop: did not find expected node ids"
    );

    const size_t second_common_node = m_splitting_path.node_id_at_position(1);
    const size_t second_last_common_node =
        m_splitting_path.node_id_at_position(m_splitting_path.number_of_edges() - 1);

    std::array<size_t, 2>
        edge_id; // an edge_id that was in a repeating path before, but will no longer be
    current = 0;

    if (type_1_face.node_id_at_position(pos[0] + 1) == second_common_node)
        edge_id[current++] =
            type_1_face.edge_id_at_position(pos[0] + type_1_face.number_of_edges() - 1);
    if (type_1_face.node_id_at_position(pos[0] + type_1_face.number_of_edges() - 1) ==
        second_common_node)
        edge_id[current++] = type_1_face.edge_id_at_position(pos[0]);
    if (type_1_face.node_id_at_position(pos[0] + 1) == second_last_common_node)
        edge_id[current++] =
            type_1_face.edge_id_at_position(pos[0] + type_1_face.number_of_edges() - 1);
    if (type_1_face.node_id_at_position(pos[0] + type_1_face.number_of_edges() - 1) ==
        second_last_common_node)
        edge_id[current++] = type_1_face.edge_id_at_position(pos[0]);

    if (type_1_face.node_id_at_position(pos[1] + 1) == second_common_node)
        edge_id[current++] =
            type_1_face.edge_id_at_position(pos[1] + type_1_face.number_of_edges() - 1);
    if (type_1_face.node_id_at_position(pos[1] + type_1_face.number_of_edges() - 1) ==
        second_common_node)
        edge_id[current++] = type_1_face.edge_id_at_position(pos[0]);
    if (type_1_face.node_id_at_position(pos[1] + 1) == second_last_common_node)
        edge_id[current++] =
            type_1_face.edge_id_at_position(pos[1] + type_1_face.number_of_edges() - 1);
    if (type_1_face.node_id_at_position(pos[1] + type_1_face.number_of_edges() - 1) ==
        second_last_common_node)
        edge_id[current++] = type_1_face.edge_id_at_position(pos[1]);

    DOMUS_ASSERT(
        current == 2,
        "repeated_path_one_common_endpoint_loop: did not find expected edge ids"
    );

    std::vector<Path> repeated_paths;
    for (const Path& old_repeated_path : m_old_type_4_face.repeated_paths()) {
        if (old_repeated_path.get_first_edge_id() == edge_id[0] ||
            old_repeated_path.get_last_edge_id() == edge_id[0])
            continue;
        if (old_repeated_path.get_first_edge_id() == edge_id[0] ||
            old_repeated_path.get_last_edge_id() == edge_id[0])
            continue;
        repeated_paths.push_back(old_repeated_path);
    }
    DOMUS_ASSERT(
        repeated_paths.size() == 1,
        "repeated_paths_both_common_endpoints: should have obtained 2 repeated paths"
    );
    return repeated_paths[0];
}

std::vector<Path> NextTypesEstablisher::repeated_paths_double_cylinder() {
    std::println("repeated_paths_double_cylinder");
    std::println("old face");
    m_old_type_4_face.print();
    std::println("splitting path");
    m_splitting_path.print();
    std::println("new face 1");
    m_face_1.print();
    std::println("new face 2");
    m_face_2.print();

    const size_t first_id = m_splitting_path.get_first_node_id();
    const size_t last_id = m_splitting_path.get_last_node_id();

    for (const Path& repeated_path : m_old_type_4_face.repeated_paths()) {
        std::optional<size_t> pos_first;
        std::optional<size_t> pos_last;
        for (size_t i = 1; i < repeated_path.number_of_edges(); ++i) {
            if (repeated_path.node_id_at_position(i) == first_id)
                pos_first = i;
            if (repeated_path.node_id_at_position(i) == last_id)
                pos_last = i;
        }

        DOMUS_ASSERT(
            pos_first.has_value() == pos_last.has_value(),
            "repeated_path_double_cylinder: found only one of the two nodes"
        );

        if (!pos_first.has_value())
            continue;

        std::vector<Path> paths;
        paths.resize(2);

        Path* current = &paths[0];
        for (size_t i = 0; i < repeated_path.number_of_edges(); ++i) {
            const size_t node_id = repeated_path.node_id_at_position(i);
            const size_t edge_id = repeated_path.edge_id_at_position(i);
            if (current)
                current->push_back(m_graph, node_id, edge_id);

            const size_t next_node_id = repeated_path.node_id_at_position(i + 1);
            if (next_node_id != first_id && next_node_id != last_id)
                continue;

            if (current != nullptr) {
                if (first_id == last_id)
                    current = &paths[1];
                else
                    current = nullptr;
            } else
                current = &paths[1];
        }
    }
    DOMUS_ASSERT(false, "repeated_path_double_cylinder: did not find new repeated path");
    return {};
}

Path NextTypesEstablisher::repeated_sub_path(
    const Path& type_1_face, const Path& repeated_path, const size_t in_between_node_id
) {
    bool ignoring_nodes = type_1_face.contains_edge_id(repeated_path.get_first_edge_id());
    Path result;

    for (size_t i = 0; i < repeated_path.number_of_edges(); i++) {
        const size_t node_id = repeated_path.node_id_at_position(i);
        const size_t edge_id = repeated_path.edge_id_at_position(i);
        if (!ignoring_nodes) {
            result.push_back(m_graph, node_id, edge_id);
        }
        const size_t next_node_id = repeated_path.node_id_at_position(i + 1);
        if (next_node_id == in_between_node_id) {
            if (ignoring_nodes)
                ignoring_nodes = false;
            else
                break;
        }
    }

    return result;
}

Path NextTypesEstablisher::repeated_path_one_common_endpoint_no_loop_type_2(
    const Path& type_1_face
) {
    const size_t first_id = m_splitting_path.get_first_node_id();
    const size_t last_id = m_splitting_path.get_last_node_id();

    std::optional<size_t> in_between_of_repeated_path;
    std::optional<const Path*> repeated_path;

    for (const Path& rep_path : m_old_type_4_face.repeated_paths()) {
        if (in_between_of_repeated_path.has_value())
            break;
        for (size_t i = 1; i < rep_path.number_of_edges(); i++) {
            const size_t node_id = rep_path.node_id_at_position(i);
            if (first_id == node_id || last_id == node_id) {
                in_between_of_repeated_path = node_id;
                repeated_path = &rep_path;
                break;
            }
        }
    }
    DOMUS_ASSERT(
        repeated_path.has_value(),
        "repeated_paths_one_common_endpoint_no_loop_type_2: did not find in between node"
    );

    return repeated_sub_path(type_1_face, *repeated_path.value(), *in_between_of_repeated_path);
}

std::vector<Path>
NextTypesEstablisher::repeated_paths_one_common_endpoint_no_loop_type_3(const Path& type_1_face) {
    std::vector<Path> repeated_paths;
    repeated_paths.reserve(2);

    const size_t first_id = m_splitting_path.get_first_node_id();
    const size_t last_id = m_splitting_path.get_last_node_id();

    std::optional<size_t> in_between_of_repeated_path;
    std::optional<const Path*> old_repeated_path;

    for (const Path& rep_path : m_old_type_4_face.repeated_paths()) {
        if (in_between_of_repeated_path.has_value())
            break;
        for (size_t i = 1; i < rep_path.number_of_edges(); i++) {
            const size_t node_id = rep_path.node_id_at_position(i);
            if (first_id == node_id || last_id == node_id) {
                in_between_of_repeated_path = node_id;
                old_repeated_path = &rep_path;
                break;
            }
        }
    }

    DOMUS_ASSERT(
        old_repeated_path.has_value(),
        "repeated_paths_one_common_endpoint_no_loop_type_3: did not find in between node"
    );

    repeated_paths.push_back(
        repeated_sub_path(type_1_face, *old_repeated_path.value(), *in_between_of_repeated_path)
    );

    for (const Path& repeated_path : m_old_type_4_face.repeated_paths()) {
        const size_t first_edge_id = repeated_path.get_first_edge_id();
        const size_t last_edge_id = repeated_path.get_last_edge_id();
        if (type_1_face.contains_edge_id(first_edge_id) ||
            type_1_face.contains_edge_id(last_edge_id))
            continue;
        repeated_paths.push_back(repeated_path);
    }

    DOMUS_ASSERT(
        repeated_paths.size() == 2,
        "repeated_paths_one_common_endpoint_no_loop_type_3: expected paths to find is 2 but is {}",
        repeated_paths.size()
    );

    return repeated_paths;
}

std::vector<Path>
NextTypesEstablisher::repeated_paths_no_common_endpoint_type_3(const Path& type_1_face) {
    return {};
    // TODO
}

// at this point we could end up with either:
// - a type 3 face and a type 1 face
// - two type 2 faces
// - a type 2 face and a type 1 face
// - two type 1 faces
std::pair<Face, Face> NextTypesEstablisher::compute_face_types(
    const Graph& graph,
    const Path& splitting_path,
    const Face& old_type_4_face,
    const Path& face_1,
    const Path& face_2
) {
    NextTypesEstablisher type_establisher(graph, splitting_path, old_type_4_face, face_1, face_2);
    return type_establisher.compute_face_types();
}

std::pair<Face, Face> NextTypesEstablisher::compute_face_types() {
    const size_t first_node_id = m_splitting_path.get_first_node_id();
    const size_t last_node_id = m_splitting_path.get_last_node_id();

    const size_t first_repeated_node_id = m_old_type_4_face.repeated_paths()[0].get_first_node_id();
    const size_t last_repeated_node_id = m_old_type_4_face.repeated_paths()[0].get_last_node_id();

    const size_t first_count_1 = node_id_count_in_path(m_face_1, first_repeated_node_id);
    const size_t last_count_1 = node_id_count_in_path(m_face_1, last_repeated_node_id);

    const size_t first_count_2 = node_id_count_in_path(m_face_2, first_repeated_node_id);
    const size_t last_count_2 = node_id_count_in_path(m_face_2, last_repeated_node_id);

    // both endpoints of splitting path coincide with endpoints of repeating paths
    if ((first_repeated_node_id == first_node_id && last_repeated_node_id == last_node_id) ||
        (first_repeated_node_id == last_node_id && last_repeated_node_id == first_node_id)) {
        // then result can either be:
        // - a type 3 face and a type 1 face (2 vertices repeated 3 times in one face)
        // - two type 1 faces (2 vertices repeated 2 times in both faces)

        if (first_count_1 == 3 && last_count_1 == 3)
            return std::make_pair(
                Face(
                    FaceType::TYPE_3,
                    Path(m_face_1),
                    repeated_paths_both_common_endpoints(m_face_2)
                ),
                Face(FaceType::TYPE_1, Path(m_face_2), {})
            );

        if (first_count_2 == 3 && last_count_2 == 3)
            return std::make_pair(
                Face(FaceType::TYPE_1, Path(m_face_1), {}),
                Face(
                    FaceType::TYPE_3,
                    Path(m_face_2),
                    repeated_paths_both_common_endpoints(m_face_1)
                )
            );

        return std::make_pair(
            Face(FaceType::TYPE_1, Path(m_face_1), {}),
            Face(FaceType::TYPE_1, Path(m_face_2), {})
        );
    }
    // only one endpoint of splittingh path coincide with endpoints of repeating paths
    if (first_node_id == first_repeated_node_id || first_node_id == last_repeated_node_id ||
        last_node_id == last_repeated_node_id || last_node_id == first_repeated_node_id) {
        // if path is a "loop" one face is type 1 and the other is type 2
        if (first_node_id == last_node_id) {
            if (first_count_1 == 1 && last_count_1 == 1)
                return std::make_pair(
                    Face(FaceType::TYPE_1, Path(m_face_1), {}),
                    Face(
                        FaceType::TYPE_2,
                        Path(m_face_2),
                        std::vector<Path>{repeated_path_one_common_endpoint_loop(m_face_1)}
                    )
                );

            if (first_count_2 == 1 && last_count_2 == 1)
                return std::make_pair(
                    Face(
                        FaceType::TYPE_2,
                        Path(m_face_1),
                        std::vector<Path>{repeated_path_one_common_endpoint_loop(m_face_2)}
                    ),
                    Face(FaceType::TYPE_1, Path(m_face_2), {})
                );

            DOMUS_ASSERT(false, "next_case_type: should have not ended up here");
        }
        // otherwise result can either be:
        // - a type 3 face and a type 1 face (one vertex repeated 3 times in one face)
        // - a type 2 face and a type 1 face (no vertex repeated 3 times in any face)
        if (first_count_1 == 3 || last_count_1 == 3)
            return std::make_pair(
                Face(
                    FaceType::TYPE_3,
                    Path(m_face_1),
                    repeated_paths_one_common_endpoint_no_loop_type_3(m_face_2)
                ),
                Face(FaceType::TYPE_1, Path(m_face_2), {})
            );
        if (first_count_2 == 3 || last_count_2 == 3)
            return std::make_pair(
                Face(FaceType::TYPE_1, Path(m_face_1), {}),
                Face(
                    FaceType::TYPE_3,
                    Path(m_face_2),
                    repeated_paths_one_common_endpoint_no_loop_type_3(m_face_1)
                )
            );

        if (first_count_1 == 2 || last_count_1 == 2)
            return std::make_pair(
                Face(
                    FaceType::TYPE_2,
                    Path(m_face_1),
                    {repeated_path_one_common_endpoint_no_loop_type_2(m_face_2)}
                ),
                Face(FaceType::TYPE_1, Path(m_face_2), {})
            );
        if (first_count_2 == 2 || last_count_2 == 2)
            return std::make_pair(
                Face(FaceType::TYPE_1, Path(m_face_1), {}),
                Face(
                    FaceType::TYPE_2,
                    Path(m_face_2),
                    {repeated_path_one_common_endpoint_no_loop_type_2(m_face_1)}
                )
            );

        DOMUS_ASSERT(false, "next_case_type: should have not ended up here");
    }
    // no endpoint of splittingh path coincide with endpoints of repeating paths
    // then result can either be:
    // - a type 3 face and a type 1 face (one face does not contain repeated vertices)
    // - two type 2 faces (both faces contain repeated vertices)

    if (first_count_1 == 1 && last_count_1 == 1)
        return std::make_pair(
            Face(FaceType::TYPE_1, Path(m_face_1), {}),
            Face(
                FaceType::TYPE_3,
                Path(m_face_2),
                repeated_paths_no_common_endpoint_type_3(m_face_1)
            )
        );

    if (first_count_2 == 1 && last_count_2 == 1)
        return std::make_pair(
            Face(
                FaceType::TYPE_3,
                Path(m_face_1),
                repeated_paths_no_common_endpoint_type_3(m_face_2)
            ),
            Face(FaceType::TYPE_1, Path(m_face_2), {})
        );

    std::vector<Path> repeated_paths = repeated_paths_double_cylinder();

    const size_t i = (m_face_1.contains_edge_id(repeated_paths[0].get_first_edge_id()));
    return {
        Face(FaceType::TYPE_2, Path(m_face_1), {std::move(repeated_paths[!i])}),
        Face(FaceType::TYPE_2, Path(m_face_2), {std::move(repeated_paths[i])})
    };
}

} // namespace domus::torus
