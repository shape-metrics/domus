#include "domus/torus/embedder.hpp"

#include <variant>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

#include "../core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/path.hpp"

namespace domus::torus {
using namespace domus::graph;

enum class FaceType {
    TYPE_1,
    TYPE_2,
    TYPE_3,
    TYPE_4,
};

// TODO
FaceType compute_type_of_face(const Path& path);

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
    pos_2 = initial_pos_2 + cycle_1.size();
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

graph::Embedding compute_embedding_of_two_cycles(
    const Graph& graph, const Cycle& cycle_1, Cycle& cycle_2, const size_t intersection_node_id
) {
    graph::Embedding embedding(graph);
    auto intersection =
        compute_intersection_between_cycles(graph, cycle_1, cycle_2, intersection_node_id);
    if (std::holds_alternative<Path>(intersection)) {
        const Path& intersection_path = std::get<Path>(intersection);
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
    } else {
        size_t pos_1 = cycle_1.node_id_position(intersection_node_id);
        size_t pos_2 = cycle_2.node_id_position(intersection_node_id);
        embedding.add_edge(
            intersection_node_id,
            cycle_1.node_id_at(pos_1 + 1),
            cycle_1.edge_id_at(pos_1)
        );
        embedding.add_edge(
            intersection_node_id,
            cycle_2.node_id_at(pos_2 + 1),
            cycle_2.edge_id_at(pos_2)
        );
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

        embedding.add_edge(
            cycle_1.node_id_at(pos_1 + 1),
            intersection_node_id,
            cycle_1.edge_id_at(pos_1)
        );
        embedding.add_edge(
            cycle_2.node_id_at(pos_2 + 1),
            intersection_node_id,
            cycle_2.edge_id_at(pos_2)
        );
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
    return embedding;
}

std::optional<graph::Embedding> compute_toroidal_embedding(
    const Graph& graph, const Cycle& cycle_1, Cycle& cycle_2, const size_t intersection_node_id
) {
    auto embedding = compute_embedding_of_two_cycles(graph, cycle_1, cycle_2, intersection_node_id);
    DOMUS_ASSERT(
        compute_embedding_genus(graph, embedding) == 1,
        "compute_toroidal_embedding: the embedding of the two cycles is not toroidal"
    );
    // auto faces = planarity::compute_faces_in_embedding(embedding);
    // DOMUS_ASSERT(
    //     faces.size() == 1,
    //     "compute_toroidal_embedding: should have obtained a single face"
    // );
    // const Path& path = faces[0];
    // FaceType face_type = compute_type_of_face(path);
    // DOMUS_ASSERT(
    //     face_type == FaceType::TYPE_3 || face_type == FaceType::TYPE_4,
    //     "compute_toroidal_embedding: face from cycles should be type 3 or 4"
    // );
    return embedding;
}

std::optional<graph::Embedding> compute_toroidal_embedding(const Graph& graph) {
    auto cycle_basis = algorithms::compute_cycle_basis(graph);
    for (size_t i = 0; i < cycle_basis.size(); ++i) {
        Cycle& cycle_1 = cycle_basis[i];
        for (size_t j = i + 1; j < cycle_basis.size(); ++j) {
            Cycle& cycle_2 = cycle_basis[j];
            std::optional<size_t> intersection_node_id =
                algorithms::do_cycles_intersect(cycle_1, cycle_2);
            if (!intersection_node_id.has_value())
                continue;
            auto embedding =
                compute_toroidal_embedding(graph, cycle_1, cycle_2, *intersection_node_id);
            if (embedding)
                return embedding;
        }
    }
    return std::nullopt;
}

} // namespace domus::torus