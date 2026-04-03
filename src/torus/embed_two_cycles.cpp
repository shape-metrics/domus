#include "embed_two_cycles.hpp"

#include <variant>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/path.hpp"

#include "faces.hpp"

namespace domus::torus {
using namespace domus::graph;

void initialize_embedding_type_4(
    const Path& intersection_path, const Cycle& cycle_1, const Cycle& cycle_2, Embedding& embedding
) {
    for (size_t pos = 0; pos < intersection_path.number_of_edges(); ++pos) {
        const size_t node_1 = intersection_path.node_id_at_position(pos);
        const size_t edge_id = intersection_path.edge_id_at_position(pos);
        const size_t node_2 = intersection_path.node_id_at_position(pos + 1);
        embedding.add_edge(node_1, node_2, edge_id);
        embedding.add_edge(node_2, node_1, edge_id);
    }
    size_t curr = intersection_path.get_last_node_id();
    size_t pos_curr = cycle_1.node_id_position(curr);
    while (cycle_1.node_id_at(pos_curr) != intersection_path.get_first_node_id()) {
        const size_t edge_id = cycle_1.edge_id_at(pos_curr);
        embedding.add_edge(curr, cycle_1.node_id_at(pos_curr + 1), edge_id);
        embedding.add_edge(cycle_1.node_id_at(pos_curr + 1), curr, edge_id);
        pos_curr++;
        curr = cycle_1.node_id_at(pos_curr);
    }
    curr = intersection_path.get_last_node_id();
    pos_curr = cycle_2.node_id_position(curr);
    while (cycle_2.node_id_at(pos_curr) != intersection_path.get_first_node_id()) {
        const size_t edge_id = cycle_2.edge_id_at(pos_curr);
        embedding.add_edge(curr, cycle_2.node_id_at(pos_curr + 1), edge_id);
        embedding.add_edge(cycle_2.node_id_at(pos_curr + 1), curr, edge_id);
        pos_curr++;
        curr = cycle_2.node_id_at(pos_curr);
    }
}

void initialize_embedding_type_3(
    const size_t intersection_node_id,
    const Cycle& cycle_1,
    const Cycle& cycle_2,
    Embedding& embedding
) {
    size_t pos_1 = cycle_1.node_id_position(intersection_node_id);
    size_t pos_2 = cycle_2.node_id_position(intersection_node_id);
    embedding
        .add_edge(intersection_node_id, cycle_1.node_id_at(pos_1 + 1), cycle_1.edge_id_at(pos_1));
    embedding
        .add_edge(intersection_node_id, cycle_2.node_id_at(pos_2 + 1), cycle_2.edge_id_at(pos_2));
    embedding.add_edge(
        intersection_node_id,
        cycle_1.node_id_at(pos_1 + cycle_1.size() - 1),
        cycle_1.edge_id_at(pos_1 + cycle_1.size() - 1)
    );
    embedding.add_edge(
        intersection_node_id,
        cycle_2.node_id_at(pos_2 + cycle_2.size() - 1),
        cycle_2.edge_id_at(pos_2 + cycle_2.size() - 1)
    );

    embedding
        .add_edge(cycle_1.node_id_at(pos_1 + 1), intersection_node_id, cycle_1.edge_id_at(pos_1));
    embedding
        .add_edge(cycle_2.node_id_at(pos_2 + 1), intersection_node_id, cycle_2.edge_id_at(pos_2));
    embedding.add_edge(
        cycle_1.node_id_at(pos_1 + cycle_1.size() - 1),
        intersection_node_id,
        cycle_1.edge_id_at(pos_1 + cycle_1.size() - 1)
    );
    embedding.add_edge(
        cycle_2.node_id_at(pos_2 + cycle_2.size() - 1),
        intersection_node_id,
        cycle_2.edge_id_at(pos_2 + cycle_2.size() - 1)
    );

    pos_1++;
    pos_2++;
    while (cycle_1.node_id_at(pos_1 + 1) != intersection_node_id) {
        embedding.add_edge(
            cycle_1.node_id_at(pos_1),
            cycle_1.node_id_at(pos_1 + 1),
            cycle_1.edge_id_at(pos_1)
        );
        embedding.add_edge(
            cycle_1.node_id_at(pos_1 + 1),
            cycle_1.node_id_at(pos_1),
            cycle_1.edge_id_at(pos_1)
        );
        pos_1++;
    }
    while (cycle_2.node_id_at(pos_2 + 1) != intersection_node_id) {
        embedding.add_edge(
            cycle_2.node_id_at(pos_2),
            cycle_2.node_id_at(pos_2 + 1),
            cycle_2.edge_id_at(pos_2)
        );
        embedding.add_edge(
            cycle_2.node_id_at(pos_2 + 1),
            cycle_2.node_id_at(pos_2),
            cycle_2.edge_id_at(pos_2)
        );
        pos_2++;
    }
}

std::variant<Path, size_t> compute_intersection_between_cycles(
    const Graph& graph, const Cycle& cycle_1, Cycle& cycle_2, const size_t intersection_node_id
) {
    size_t initial_pos_1 = cycle_1.node_id_position(intersection_node_id);
    size_t initial_pos_2 = cycle_2.node_id_position(intersection_node_id);

    if (cycle_2.node_id_at(initial_pos_2 + cycle_2.size() - 1) ==
        cycle_1.node_id_at(initial_pos_1 + 1)) {
        cycle_2.reverse();
        initial_pos_2 = cycle_2.size() - initial_pos_2;
    }

    size_t next_1 = cycle_1.node_id_at(initial_pos_1 + 1);
    size_t next_2 = cycle_2.node_id_at(initial_pos_2 + 1);
    size_t prev_1 = cycle_1.node_id_at(initial_pos_1 + cycle_1.size() - 1);
    size_t prev_2 = cycle_2.node_id_at(initial_pos_2 + cycle_2.size() - 1);

    if (next_1 != next_2 && prev_1 != prev_2)
        return intersection_node_id; // intersection is just a node

    Path intersection_path;
    size_t curr = intersection_node_id;
    size_t pos_1 = initial_pos_1;
    size_t pos_2 = initial_pos_2;
    while (next_1 == next_2) {
        size_t edge_id = cycle_1.edge_id_at(pos_1);
        DOMUS_ASSERT(
            cycle_2.edge_id_at(pos_2) == edge_id,
            "compute_intersection_between_cycles: edges_ids in cycle do not match"
        );
        pos_1++;
        pos_2++;
        intersection_path.push_back(graph, curr, edge_id);
        curr = cycle_1.node_id_at(pos_1);
        next_1 = cycle_1.node_id_at(pos_1 + 1);
        next_2 = cycle_2.node_id_at(pos_2 + 1);
    }
    curr = intersection_node_id;
    pos_1 = initial_pos_1 + cycle_1.size();
    pos_2 = initial_pos_2 + cycle_2.size();
    while (prev_1 == prev_2) {
        size_t edge_id = cycle_1.edge_id_at(pos_1 - 1);
        DOMUS_ASSERT(
            cycle_2.edge_id_at(pos_2 - 1) == edge_id,
            "compute_intersection_between_cycles: edges_ids in cycle do not match"
        );
        pos_1--;
        pos_2--;
        intersection_path.push_front(graph, curr, edge_id);
        curr = cycle_1.node_id_at(pos_1);
        prev_1 = cycle_1.node_id_at(pos_1 - 1);
        prev_2 = cycle_2.node_id_at(pos_2 - 1);
    }
    return intersection_path;
}

Path path_from_cycle_incrementing_position(
    const Graph& graph, const Cycle& cycle, const size_t first_id, const size_t last_id
) {
    Path path;
    size_t pos = cycle.node_id_position(first_id);
    path.push_back(graph, first_id, cycle.edge_id_at(pos));
    pos++;
    while (cycle.node_id_at(pos) != last_id) {
        path.push_back(graph, cycle.node_id_at(pos), cycle.edge_id_at(pos));
        pos++;
    }
    DOMUS_ASSERT(
        path.get_first_node_id() == first_id && path.get_last_node_id() == last_id,
        "path_from_cycle_incrementing_position: path does not start and end at the expected nodes"
    );
    return path;
}

std::vector<Path> compute_repeated_subpaths_of_face(
    const Graph& graph,
    const Cycle& cycle_1,
    const Cycle& cycle_2,
    const size_t first_id,
    const size_t last_id
) {
    std::vector<Path> repeated_paths;
    if (first_id == last_id) {
        repeated_paths.push_back(
            path_from_cycle_incrementing_position(graph, cycle_1, first_id, last_id)
        );
        repeated_paths.push_back(
            path_from_cycle_incrementing_position(graph, cycle_2, first_id, last_id)
        );
        return repeated_paths;
    }
    repeated_paths.push_back(
        path_from_cycle_incrementing_position(graph, cycle_1, first_id, last_id)
    );
    repeated_paths.push_back(
        path_from_cycle_incrementing_position(graph, cycle_1, last_id, first_id)
    );
    repeated_paths[1].reverse();

    Path other_path = path_from_cycle_incrementing_position(graph, cycle_2, first_id, last_id);

    if (other_path == repeated_paths[0] || other_path == repeated_paths[1]) {
        other_path = path_from_cycle_incrementing_position(graph, cycle_2, last_id, first_id);
        other_path.reverse();
        DOMUS_ASSERT(
            other_path != repeated_paths[0] && other_path != repeated_paths[1],
            "compute_repeated_paths: other_path is equal to one of the repeated paths"
        );
    }
    repeated_paths.push_back(std::move(other_path));
    return repeated_paths;
}

std::pair<Embedding, Face> compute_embedding_of_two_cycles(
    const Graph& graph, const Cycle& cycle_1, Cycle& cycle_2, const size_t intersection_node_id
) {
    Embedding embedding(graph);
    FaceType face_type;
    size_t first_id;
    size_t last_id;
    const auto intersection =
        compute_intersection_between_cycles(graph, cycle_1, cycle_2, intersection_node_id);
    if (std::holds_alternative<Path>(intersection)) {
        const Path& intersection_path = std::get<Path>(intersection);
        initialize_embedding_type_4(intersection_path, cycle_1, cycle_2, embedding);
        first_id = intersection_path.get_first_node_id();
        last_id = intersection_path.get_last_node_id();
        face_type = FaceType::TYPE_4;
    } else {
        initialize_embedding_type_3(intersection_node_id, cycle_1, cycle_2, embedding);
        first_id = intersection_node_id;
        last_id = intersection_node_id;
        face_type = FaceType::TYPE_3;
    }
    DOMUS_ASSERT(
        compute_embedding_genus(embedding) == 1,
        "compute_embedding_of_two_cycles: the embedding of the two cycles is not toroidal"
    );
    std::vector<Path> repeated_paths =
        compute_repeated_subpaths_of_face(graph, cycle_1, cycle_2, first_id, last_id);
    std::vector<Path> faces = graph::compute_faces_in_embedding(graph, embedding);
    DOMUS_ASSERT(
        faces.size() == 1,
        "compute_embedding_of_two_cycles: should have obtained a single face"
    );
    DOMUS_ASSERT(
        (repeated_paths.size() == 2 && face_type == FaceType::TYPE_3) ||
            (repeated_paths.size() == 3 && face_type == FaceType::TYPE_4),
        "compute_embedding_of_two_cycles: repeated paths do not match face type"
    );
    return {embedding, Face(face_type, std::move(faces[0]), std::move(repeated_paths))};
}

} // namespace domus::torus