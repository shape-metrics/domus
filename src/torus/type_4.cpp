#include "domus/torus/type_4.hpp"

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/torus/bridge.hpp"

#include <ranges>

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

auto candidate_face_splitting_paths(const Graph& graph, const Bridge& bridge) {
    return all_pairs_of_view(compute_all_feet_in_bridge(bridge)) |
           std::views::transform([&](const auto& pair) {
               const auto& foot_1 = pair.first;
               const auto& foot_2 = pair.second;
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

void handle_type_4(const Graph& graph, const std::vector<Bridge>& bridges) {
    for (const Bridge& bridge : bridges) {
        bridge.print();
        if (bridge.get_bridge().get_number_of_nodes() == 2) {
            const Path path = path_of_chord(graph, bridge);
            path.print();
        } else {
            auto paths = candidate_face_splitting_paths(graph, bridge);
            for (const Path& path : paths)
                path.print();
        }
    }
}

void handle_type_4(const Graph& graph, const Embedding& embedding) {
    handle_type_4(graph, Bridge::compute(graph, embedding));
}

} // namespace domus::torus