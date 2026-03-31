#include "bridge.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

namespace domus::torus {
using namespace domus::graph;

void chord_bridge(const Bridge& chord) {}

void normal_bridge(const Bridge& bridge) {
    const Graph& graph = bridge.get_bridge();
    const std::vector<EdgeId> feet =
        std::ranges::to<std::vector<EdgeId>>(compute_all_feet_in_bridge(bridge));
    auto pairs =
        feet | std::views::enumerate | std::views::transform([&feet](auto&& tuple) {
            auto [i, first] = tuple;
            // For each element at i, create pairs with elements from i+1 to end
            return feet | std::views::drop(i + 1) | std::views::transform([first](auto&& second) {
                       return std::make_pair(first, second);
                   });
        }) |
        std::views::join; // flatten

    for (auto const& [foot_1, foot_2] : pairs) {
        const size_t from_id_1 = foot_1.edge.from_id;
        const size_t to_id_1 = foot_1.edge.to_id;
        const size_t attachment_1 = (bridge.is_attachment(from_id_1)) ? from_id_1 : to_id_1;
        const size_t inner_1 = (attachment_1 == from_id_1) ? to_id_1 : from_id_1;

        const size_t from_id_2 = foot_2.edge.from_id;
        const size_t to_id_2 = foot_2.edge.to_id;
        const size_t attachment_2 = (bridge.is_attachment(from_id_2)) ? from_id_2 : to_id_2;
        const size_t inner_2 = (attachment_2 == from_id_2) ? to_id_2 : from_id_2;

        Path path = algorithms::find_shortest_path_between_nodes(graph, inner_1, inner_2).value();
        path.push_front(graph, inner_1, foot_1.id);
        path.push_back(graph, inner_2, foot_2.id);
    }
}

void handle_type_3(std::vector<Bridge>& bridges) {
    for (const Bridge& bridge : bridges) {
        if (bridge.get_bridge().get_number_of_nodes() == 2) { // is a chord
            chord_bridge(bridge);
        } else {
            const std::vector<EdgeId> feet =
                std::ranges::to<std::vector<EdgeId>>(compute_all_feet_in_bridge(bridge));
            normal_bridge(bridge);
        }
    }
}

} // namespace domus::torus