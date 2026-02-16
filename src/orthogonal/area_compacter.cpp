#include "domus/orthogonal/area_compacter.hpp"

#include <algorithm>
#include <limits.h>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/utils.hpp"
#include "domus/orthogonal/drawing.hpp"

class UndirectedGraph;

using namespace std;

auto build_index_to_nodes_map(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    auto [node_to_index_x, node_to_index_y] = compute_node_to_index_position(graph, attributes);
    unordered_map<int, unordered_set<int>> index_x_to_nodes;
    for (const auto& [node_id, index] : node_to_index_x)
        index_x_to_nodes[index].insert(node_id);
    unordered_map<int, unordered_set<int>> index_y_to_nodes;
    for (const auto& [node_id, index] : node_to_index_y)
        index_y_to_nodes[index].insert(node_id);
    return make_tuple(index_x_to_nodes, node_to_index_x, index_y_to_nodes, node_to_index_y);
}

bool can_move_to_prev_index(IntPairHashSet& prev, IntPairHashSet& to_shift) {
    if (to_shift.size() != 1)
        throw runtime_error("can_move_left: wtf");
    auto [to_shift_min, to_shift_max] = *to_shift.begin();
    for (auto [prev_min, prev_max] : prev)
        if (!(prev_min > to_shift_max || to_shift_min > prev_max))
            return false;
    return true;
}

int compute_shift_amount(
    int index, unordered_map<int, IntPairHashSet>& index_to_min_max_coordinate
) {
    int shift = 0;
    IntPairHashSet& to_shift = index_to_min_max_coordinate[index];
    while (true) {
        if (index - shift == 0)
            return shift;
        IntPairHashSet& prev = index_to_min_max_coordinate[index - shift - 1];
        if (can_move_to_prev_index(prev, to_shift))
            shift++;
        else
            break;
    }
    return shift;
}

auto build_index_x_to_min_max_index_y(
    unordered_map<int, unordered_set<int>>& index_x_to_nodes,
    unordered_map<int, int>& node_to_index_y
) {
    unordered_map<int, IntPairHashSet> index_to_min_max_y;
    for (const auto& [index, nodes] : index_x_to_nodes) {
        int min_y = INT_MAX;
        int max_y = 0;
        for (int node_id : nodes) {
            int y = node_to_index_y[node_id];
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y);
        }
        index_to_min_max_y[index] = {{min_y, max_y}};
    }
    return index_to_min_max_y;
}

auto build_index_y_to_min_max_index_x(
    unordered_map<int, unordered_set<int>>& index_to_nodes, unordered_map<int, int>& node_to_index_x
) {
    unordered_map<int, IntPairHashSet> index_to_min_max_x;
    for (const auto& [index, nodes] : index_to_nodes) {
        int min_x = INT_MAX;
        int max_x = 0;
        for (int node_id : nodes) {
            int x = node_to_index_x[node_id];
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
        }
        index_to_min_max_x[index] = {{min_x, max_x}};
    }
    return index_to_min_max_x;
}

void compact_area(const UndirectedGraph& graph, GraphAttributes& attributes) {
    auto [index_x_to_nodes, nodes_to_index_x, index_y_to_nodes, nodes_to_index_y] =
        build_index_to_nodes_map(graph, attributes);
    // compacting x
    auto index_to_min_max_y = build_index_x_to_min_max_index_y(index_x_to_nodes, nodes_to_index_y);
    int index = 0;
    while (index_to_min_max_y.contains(index + 1)) {
        ++index;
        int shift_amount = compute_shift_amount(index, index_to_min_max_y);
        if (shift_amount == 0) {
            continue;
        }
        unordered_set<int>& nodes_to_shift = index_x_to_nodes[index];
        for (int node_id : nodes_to_shift) {
            int old_x = attributes.get_position_x(node_id);
            attributes.change_position_x(node_id, old_x - 100 * shift_amount);
        }
        index_to_min_max_y[index - shift_amount].insert(*index_to_min_max_y[index].begin());
        index_to_min_max_y[index].clear();
    }
    // compacting y
    auto index_to_min_max_x = build_index_y_to_min_max_index_x(index_y_to_nodes, nodes_to_index_x);
    index = 0;
    while (index_to_min_max_x.contains(index + 1)) {
        ++index;
        int shift_amount = compute_shift_amount(index, index_to_min_max_x);
        if (shift_amount == 0)
            continue;
        unordered_set<int>& nodes_to_shift = index_y_to_nodes[index];
        for (int node_id : nodes_to_shift) {
            int old_y = attributes.get_position_y(node_id);
            attributes.change_position_y(node_id, old_y - 100 * shift_amount);
        }
        index_to_min_max_x[index - shift_amount].insert(*index_to_min_max_x[index].begin());
        index_to_min_max_x[index].clear();
    }
}