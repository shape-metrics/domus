#include "domus/torus/type_4.hpp"

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/torus/bridge.hpp"

#include "faces.hpp"

namespace domus::torus {
using namespace domus::graph;

Path path_of_chord(const Graph& graph, const Bridge& chord) {
    const size_t node_1 = chord.get_new_id_to_old_id().get_label(0);
    const size_t edge_id = chord.get_new_edge_id_to_old_id().get_label(0);
    Path path;
    path.push_back(graph, node_1, edge_id);
    return path;
}

auto all_pairs_of_view(auto&& feet) {
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

auto candidate_face_splitting_paths_in_graph(const Graph& graph, const Bridge& bridge) {
    return all_pairs_of_view(compute_all_feet_in_bridge(bridge)) |
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

void augment_embedding_with_path(Embedding& embedding, const Path& path) {
    for (size_t i = 1; i < path.number_of_edges() - 1; ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.add_edge(node_id_1, node_id_2, edge_id);
        embedding.add_edge(node_id_2, node_id_1, edge_id);
    }
    if (path.number_of_edges() > 1) {
        const size_t first_node_id = path.get_first_node_id();
        const size_t second_node_id = path.node_id_at_position(1);
        const size_t first_edge_id = path.edge_id_at_position(0);
        embedding.add_edge(second_node_id, first_node_id, first_edge_id);

        const size_t last_node_id = path.get_last_node_id();
        const size_t second_last_node_id = path.node_id_at_position(path.number_of_edges() - 1);
        const size_t last_edge_id = path.edge_id_at_position(path.number_of_edges() - 1);
        embedding.add_edge(second_last_node_id, last_node_id, last_edge_id);
    }
}

void remove_augment_of_path_in_embedding(Embedding& embedding, const Path& path) {
    for (size_t i = 1; i < path.number_of_edges() - 1; ++i) {
        const size_t node_id_1 = path.node_id_at_position(i);
        const size_t node_id_2 = path.node_id_at_position(i + 1);
        const size_t edge_id = path.edge_id_at_position(i);
        embedding.remove_edge(node_id_1, node_id_2, edge_id);
        embedding.remove_edge(node_id_2, node_id_1, edge_id);
    }
    if (path.number_of_edges() > 1) {
        const size_t first_node_id = path.get_first_node_id();
        const size_t second_node_id = path.node_id_at_position(1);
        const size_t first_edge_id = path.edge_id_at_position(0);
        embedding.remove_edge(second_node_id, first_node_id, first_edge_id);

        const size_t last_node_id = path.get_last_node_id();
        const size_t second_last_node_id = path.node_id_at_position(path.number_of_edges() - 1);
        const size_t last_edge_id = path.edge_id_at_position(path.number_of_edges() - 1);
        embedding.remove_edge(second_last_node_id, last_node_id, last_edge_id);
    }
}

bool did_path_split(
    const Graph& graph, const Embedding& embedding, const Face& initial_face, const Path& path
) {
    const size_t first_of_path = path.get_first_node_id();
    const size_t last_of_path = path.get_last_node_id();

    const size_t first_repeated_id = initial_face.repeated_paths()[0].get_first_node_id();
    const size_t last_repeated_id = initial_face.repeated_paths()[0].get_last_node_id();

    if ((first_of_path == first_repeated_id && last_of_path == last_repeated_id) ||
        (first_of_path == last_repeated_id && last_of_path == first_repeated_id))
        return true;

    std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
    DOMUS_ASSERT(faces.size() == 2, "did_path_split: embedding should have exactly 2 faces");
    for (const Path& face : faces) {
        size_t first_count = 0;
        size_t last_count = 0;
        for (size_t i = 0; i < face.number_of_edges(); ++i) {
            const size_t node_id = face.node_id_at_position(i);
            if (first_repeated_id == node_id)
                first_count++;
            if (last_repeated_id == node_id)
                last_count++;
        }
        if (first_count == 3 && last_count == 3)
            return false;
    }
    return true;
}

void try_face_splits_with_path(
    const Graph& graph, const Path& path, Embedding& embedding, const Face& face
) {
    const size_t first_id = path.get_first_node_id();
    const size_t last_id = path.get_last_node_id();
    const size_t n_edges = path.number_of_edges();

    DOMUS_ASSERT(n_edges > 0, "try_face_splits_with_path: empty path");
    DOMUS_ASSERT(
        embedding.get_degree_of_node(first_id) > 1,
        "try_face_splits_with_path: first node degree <= 1"
    );
    DOMUS_ASSERT(
        embedding.get_degree_of_node(last_id) > 1,
        "try_face_splits_with_path: last node degree <= 1"
    );

    std::vector<EdgeIter> first_edges;
    for (const EdgeIter e : embedding.get_edges(first_id))
        first_edges.push_back(e);

    std::vector<EdgeIter> last_edges;
    for (const EdgeIter e : embedding.get_edges(last_id))
        last_edges.push_back(e);

    augment_embedding_with_path(embedding, path);

    const size_t second_id = path.node_id_at_position(1);
    const size_t second_last_id = path.node_id_at_position(n_edges - 1);
    const size_t first_edge_id = path.edge_id_at_position(0);
    const size_t last_edge_id = path.edge_id_at_position(n_edges - 1);

    for (const EdgeIter edge1 : first_edges) {
        for (const EdgeIter edge2 : last_edges) {
            embedding.add_edge_after(first_id, second_id, first_edge_id, edge1.id);
            embedding.add_edge_after(last_id, second_last_id, last_edge_id, edge2.id);

            if (!did_path_split(graph, embedding, face, path)) {
                embedding.remove_edge(first_id, second_id, first_edge_id);
                embedding.remove_edge(last_id, second_last_id, last_edge_id);
                continue;
            }

            // TODO: go on with type 3

            embedding.remove_edge(first_id, second_id, first_edge_id);
            embedding.remove_edge(last_id, second_last_id, last_edge_id);
        }
    }

    remove_augment_of_path_in_embedding(embedding, path);
}

void handle_type_4(
    const Graph& graph,
    const std::vector<Bridge>& bridges,
    const Embedding& embedding,
    const Face& face
) {
    Embedding emb_cpy = embedding;

    // trying path actually in the graph
    for (const Bridge& bridge : bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(graph, bridge);
            try_face_splits_with_path(graph, path, emb_cpy, face);
        } else {
            auto paths = candidate_face_splitting_paths_in_graph(graph, bridge);
            for (const Path& path : paths) {
                try_face_splits_with_path(graph, path, emb_cpy, face);
            }
        }
    }

    // trying paths (edges) not in the graph
}

void handle_type_4(const Graph& graph, const Embedding& embedding, const Face& face) {
    handle_type_4(graph, Bridge::compute(graph, embedding), embedding, face);
}

} // namespace domus::torus
