#pragma once

#include "domus/core/graph/graphs_algorithms.hpp"

#include <stack>

namespace domus::graph::algorithms {

template <UndirectedGraphLike G> size_t compute_number_of_connected_components(const G& graph) {
    utilities::NodesContainer visited(graph);
    size_t components = 0;
    const std::function<void(size_t)> explore_component = [&](size_t start_node_id) {
        std::stack<size_t> stack;
        stack.push(start_node_id);
        while (!stack.empty()) {
            size_t node_id = stack.top();
            stack.pop();
            if (!visited.has_node(node_id)) {
                visited.add_node(node_id);
                for (const size_t neighbor_id : graph.get_neighbors(node_id))
                    if (!visited.has_node(neighbor_id))
                        stack.push(neighbor_id);
            }
        }
    };
    for (size_t node_id : graph.get_nodes_ids()) {
        if (!visited.has_node(node_id)) {
            components++;
            explore_component(node_id);
        }
    }
    return components;
}

} // namespace domus::graph::algorithms