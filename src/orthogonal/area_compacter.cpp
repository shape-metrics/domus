#include "domus/orthogonal/area_compacter.hpp"

#include <algorithm>
#include <cassert>
#include <limits.h>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "domus/core/containers.hpp"
#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/orthogonal/drawing.hpp"

class UndirectedGraph;

using namespace std;

auto build_index_to_nodes_map(const UndirectedGraph& graph, const GraphAttributes& attributes) {
    auto [node_to_index_x, node_to_index_y] = compute_node_to_index_position(graph, attributes);
    unordered_map<int, NodesContainer> index_x_to_nodes;
    node_to_index_x.for_each([&index_x_to_nodes](int node_id, int index) {
        index_x_to_nodes[index].add_node(node_id);
    });
    unordered_map<int, NodesContainer> index_y_to_nodes;
    node_to_index_y.for_each([&index_y_to_nodes](int node_id, int index) {
        index_y_to_nodes[index].add_node(node_id);
    });
    return make_tuple(
        std::move(index_x_to_nodes),
        std::move(node_to_index_x),
        std::move(index_y_to_nodes),
        std::move(node_to_index_y)
    );
}

bool can_move_to_prev_index(PairIntHashSet& prev, PairIntHashSet& to_shift) {
    assert(to_shift.size() == 1);
    int to_shift_min = 0;
    int to_shift_max = 0;
    prev.for_each([&](int min, int max) {
        to_shift_min = min;
        to_shift_max = max;
    });
    bool can_move = true;
    prev.for_each([&](int prev_min, int prev_max) {
        if (!(prev_min > to_shift_max || to_shift_min > prev_max))
            can_move = false;
    });
    return can_move;
}

int compute_shift_amount(
    int index, unordered_map<int, PairIntHashSet>& index_to_min_max_coordinate
) {
    int shift = 0;
    PairIntHashSet& to_shift = index_to_min_max_coordinate[index];
    while (true) {
        if (index - shift == 0)
            return shift;
        PairIntHashSet& prev = index_to_min_max_coordinate[index - shift - 1];
        if (can_move_to_prev_index(prev, to_shift))
            shift++;
        else
            break;
    }
    return shift;
}

auto build_index_x_to_min_max_index_y(
    unordered_map<int, NodesContainer>& index_x_to_nodes, Int_ToInt_HashMap& node_to_index_y
) {
    unordered_map<int, PairIntHashSet> index_to_min_max_y;
    for (const auto& [index, nodes] : index_x_to_nodes) {
        int min_y = INT_MAX;
        int max_y = 0;
        nodes.for_each([&](int node_id) {
            int y = node_to_index_y.get(node_id);
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y);
        });
        index_to_min_max_y[index].add(min_y, max_y);
    }
    return index_to_min_max_y;
}

auto build_index_y_to_min_max_index_x(
    unordered_map<int, NodesContainer>& index_to_nodes, Int_ToInt_HashMap& node_to_index_x
) {
    unordered_map<int, PairIntHashSet> index_to_min_max_x;
    for (const auto& [index, nodes] : index_to_nodes) {
        int min_x = INT_MAX;
        int max_x = 0;
        nodes.for_each([&](int node_id) {
            int x = node_to_index_x.get(node_id);
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
        });
        index_to_min_max_x[index].add(min_x, max_x);
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
        index_x_to_nodes[index].for_each([&](int node_id) {
            int old_x = attributes.get_position_x(node_id);
            attributes.change_position_x(node_id, old_x - 100 * shift_amount);
        });
        bool added = false;
        index_to_min_max_y[index].for_each([&](int min, int max) {
            if (added)
                return;
            index_to_min_max_y[index - shift_amount].add(min, max);
            added = true;
        });
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
        index_y_to_nodes[index].for_each([&](int node_id) {
            int old_y = attributes.get_position_y(node_id);
            attributes.change_position_y(node_id, old_y - 100 * shift_amount);
        });
        bool added = false;
        index_to_min_max_x[index].for_each([&](int min, int max) {
            if (added)
                return;
            index_to_min_max_x[index - shift_amount].add(min, max);
            added = true;
        });
        index_to_min_max_x[index].clear();
    }
}