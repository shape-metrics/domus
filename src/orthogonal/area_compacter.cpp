#include "domus/orthogonal/area_compacter.hpp"

#include <limits.h>
#include <tuple>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/drawing.hpp"

#include "../core/domus_debug.hpp"

namespace domus::orthogonal {

auto build_index_to_nodes(const Graph& graph, const GraphAttributes& attributes) {
    const auto [node_to_index_x, node_to_index_y] =
        compute_node_to_index_position(graph, attributes);
    std::vector<std::vector<size_t>> index_x_to_nodes;
    graph.for_each_node([&](size_t node_id) {
        size_t index = node_to_index_x[node_id];
        if (index_x_to_nodes.size() <= index)
            index_x_to_nodes.resize(index + 1);
        index_x_to_nodes[index].push_back(node_id);
    });

    std::vector<std::vector<size_t>> index_y_to_nodes;
    graph.for_each_node([&](size_t node_id) {
        size_t index = node_to_index_y[node_id];
        if (index_y_to_nodes.size() <= index)
            index_y_to_nodes.resize(index + 1);
        index_y_to_nodes[index].push_back(node_id);
    });

    return make_tuple(
        std::move(index_x_to_nodes),
        std::move(node_to_index_x),
        std::move(index_y_to_nodes),
        std::move(node_to_index_y)
    );
}

bool can_move_to_prev_index(
    std::vector<std::pair<size_t, size_t>>& prev, std::vector<std::pair<size_t, size_t>>& to_shift
) {
    DOMUS_ASSERT(
        to_shift.size() == 1,
        "can_move_to_prev_index: internal error happened during area compaction"
    );
    size_t to_shift_min = 0;
    size_t to_shift_max = 0;
    for (auto [min, max] : to_shift) {
        to_shift_min = min;
        to_shift_max = max;
    }
    for (auto [prev_min, prev_max] : prev)
        if (!(prev_min > to_shift_max || to_shift_min > prev_max))
            return false;
    return true;
}

size_t compute_shift_amount(
    size_t index, std::vector<std::vector<std::pair<size_t, size_t>>>& index_to_min_max_coordinate
) {
    size_t shift = 0;
    std::vector<std::pair<size_t, size_t>>& to_shift = index_to_min_max_coordinate[index];
    while (true) {
        if (index - shift == 0)
            return shift;
        std::vector<std::pair<size_t, size_t>>& prev =
            index_to_min_max_coordinate[index - shift - 1];
        if (can_move_to_prev_index(prev, to_shift))
            shift++;
        else
            break;
    }
    return shift;
}

auto build_index_to_min_max_index(
    std::vector<std::vector<size_t>>& index_to_nodes, std::vector<size_t>& node_to_other_index
) {
    std::vector<std::vector<std::pair<size_t, size_t>>> index_to_min_max_x;
    for (size_t index = 0; index < index_to_nodes.size(); ++index) {
        if (index_to_min_max_x.size() <= index)
            index_to_min_max_x.resize(index + 1);
        if (index_to_nodes[index].empty())
            continue;
        size_t min_x = INT_MAX;
        size_t max_x = 0;
        for (size_t node_id : index_to_nodes[index]) {
            size_t x = node_to_other_index[node_id];
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
        }
        index_to_min_max_x[index].push_back({min_x, max_x});
    }
    return index_to_min_max_x;
}

void compact_area(const Graph& graph, GraphAttributes& attributes) {
    auto [index_x_to_nodes, nodes_to_index_x, index_y_to_nodes, nodes_to_index_y] =
        build_index_to_nodes(graph, attributes);
    // compacting x
    auto index_to_min_max_y = build_index_to_min_max_index(index_x_to_nodes, nodes_to_index_y);
    size_t index = 0;
    while (index + 1 < index_to_min_max_y.size()) {
        ++index;
        size_t shift_amount = compute_shift_amount(index, index_to_min_max_y);
        if (shift_amount == 0)
            continue;
        for (size_t node_id : index_x_to_nodes[index]) {
            int old_x = attributes.get_position_x(node_id);
            attributes.change_position_x(node_id, old_x - 100 * static_cast<int>(shift_amount));
        }
        for (auto [min, max] : index_to_min_max_y[index]) {
            index_to_min_max_y[index - shift_amount].push_back({min, max});
            break;
        }
        index_to_min_max_y[index].clear();
    }
    // compacting y
    auto index_to_min_max_x = build_index_to_min_max_index(index_y_to_nodes, nodes_to_index_x);
    index = 0;
    while (index + 1 < index_to_min_max_x.size()) {
        ++index;
        size_t shift_amount = compute_shift_amount(index, index_to_min_max_x);
        if (shift_amount == 0)
            continue;
        for (size_t node_id : index_y_to_nodes[index]) {
            int old_y = attributes.get_position_y(node_id);
            attributes.change_position_y(node_id, old_y - 100 * static_cast<int>(shift_amount));
        }
        for (auto [min, max] : index_to_min_max_x[index]) {
            index_to_min_max_x[index - shift_amount].push_back({min, max});
            break;
        }
        index_to_min_max_x[index].clear();
    }
}

} // namespace domus::orthogonal