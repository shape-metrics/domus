#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "domus/core/graph/graph.hpp"

namespace domus::graph {
class Cycle;
}

#include <functional>
#include <iterator>

namespace domus::planarity {
class Segment;

/**
 * @brief Efficiently detects interlacement (conflicts) between attachments of pieces along a cycle.
 *
 * Precomputes cycle intervals for a reference piece in O(cycle_size), then tests any candidate piece
 * in O(K1 + K2) using a sliding window without scanning the cycle or performing dynamic memory allocations.
 */
class CycleConflictDetector {
    std::vector<int> m_cycle_labels;
    size_t m_number_of_labels = 0;
    std::vector<int> m_labels;

  public:
    CycleConflictDetector() = default;

    CycleConflictDetector(size_t cycle_size, std::span<const size_t> pos_1) {
        init(cycle_size, pos_1);
    }

    void init(size_t cycle_size, std::span<const size_t> pos_1);

    bool in_conflict(std::span<const size_t> pos_2);
};

/**
 * @brief Tests if two sets of attachment positions on a cycle conflict (interlace).
 */
inline bool are_attachments_in_conflict(
    size_t cycle_size,
    std::span<const size_t> pos_1,
    std::span<const size_t> pos_2
) {
    CycleConflictDetector detector(cycle_size, pos_1);
    return detector.in_conflict(pos_2);
}

/**
 * @brief Computes conflicts (interlacements) between items along a cycle.
 *
 * Calls on_conflict(i, j) for every pair (i, j) with i < j whose attachments interlace.
 */
template <typename Range, typename ConflictCallback, typename Projection = std::identity>
void compute_conflicts(
    size_t cycle_size,
    const Range& items,
    ConflictCallback&& on_conflict,
    Projection proj = {}
) {
    const size_t count = static_cast<size_t>(std::size(items));
    if (count <= 1)
        return;
    for (size_t i = 0; i < count - 1; ++i) {
        CycleConflictDetector detector(cycle_size, std::invoke(proj, items[i]));
        for (size_t j = i + 1; j < count; ++j) {
            if (detector.in_conflict(std::invoke(proj, items[j])))
                on_conflict(i, j);
        }
    }
}

void compute_conflicts(
    const std::vector<Segment>& segments,
    const graph::Cycle& cycle,
    graph::Graph& interlacement_graph
);

graph::Graph
compute_interlacement_graph(const std::vector<Segment>& segments, const graph::Cycle& cycle);

} // namespace domus::planarity
