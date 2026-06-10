#pragma once

#include <ranges>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"
#include "domus/torus/bridge.hpp"
#include "domus/torus/faces.hpp"

namespace domus::torus {

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

bool are_attachments_in_same_repeated_path(
    const graph::utilities::NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
    const size_t attachment_id_1,
    const size_t attachment_id_2
);

graph::Path path_of_chord(const graph::Graph& graph, const Bridge& chord);

void augment_embedding_with_path(graph::Embedding& embedding, const graph::Path& path);

void remove_augment_of_path_in_embedding(graph::Embedding& embedding, const graph::Path& path);

bool next_case(graph::Embedding& embedding, graph::Graph& graph, const Face& original_face);

void add_log_final_configuration(const std::vector<Face>& faces);

/**
 * This function is meant to be called before executing Case 3 and Case 4, for debug purposes.
 * In both cases, the repeated paths in the input face are expected to have the same endpoints,
 * and this function checks that.
 */
bool is_initial_face_valid(const Face& face);

} // namespace domus::torus
