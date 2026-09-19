#include "domus/planarity/interlacement.hpp"

#include <algorithm>
#include <cstddef>

#include "domus/core/debug.hpp"
#include "domus/core/graph/cycle.hpp"
#include "segment.hpp"

namespace domus::planarity {
using domus::graph::Cycle;

void CycleConflictDetector::init(size_t cycle_size, std::span<const size_t> pos_1) {
    if (pos_1.size() < 2) {
        m_number_of_labels = 0;
        return;
    }

    std::vector<size_t> sorted_pos(pos_1.begin(), pos_1.end());
    if (!std::ranges::is_sorted(sorted_pos)) {
        std::ranges::sort(sorted_pos);
    }
    auto [first, last] = std::ranges::unique(sorted_pos);
    sorted_pos.erase(first, last);

    if (sorted_pos.size() < 2) {
        m_number_of_labels = 0;
        return;
    }

    const size_t total_attachments = sorted_pos.size();
    m_number_of_labels = 2 * total_attachments;
    m_cycle_labels.assign(cycle_size, 0);
    m_labels.assign(m_number_of_labels, 0);

    const int wrap_label = static_cast<int>(2 * total_attachments - 1);

    // Positions strictly before the first attachment get wrap_label
    std::fill(
        m_cycle_labels.begin(),
        m_cycle_labels.begin() + static_cast<std::ptrdiff_t>(sorted_pos[0]),
        wrap_label
    );

    for (size_t i = 0; i < total_attachments; ++i) {
        const size_t curr = sorted_pos[i];
        m_cycle_labels[curr] = 2 * static_cast<int>(i);

        const size_t next = (i + 1 < total_attachments) ? sorted_pos[i + 1] : cycle_size;
        const int path_label =
            (i + 1 < total_attachments) ? (2 * static_cast<int>(i) + 1) : wrap_label;

        std::fill(
            m_cycle_labels.begin() + static_cast<std::ptrdiff_t>(curr + 1),
            m_cycle_labels.begin() + static_cast<std::ptrdiff_t>(next),
            path_label
        );
    }
}

bool CycleConflictDetector::in_conflict(std::span<const size_t> pos_2) {
    if (m_number_of_labels == 0 || pos_2.size() < 2)
        return false;

    int sum = 0;
    for (const size_t att_idx : pos_2) {
        DOMUS_ASSERT(
            att_idx < m_cycle_labels.size(),
            "CycleConflictDetector::in_conflict: attachment index out of cycle bounds"
        );
        const size_t label_idx = static_cast<size_t>(m_cycle_labels[att_idx]);
        if (m_labels[label_idx] == 0) {
            m_labels[label_idx] = 1;
            ++sum;
        }
    }

    int part_sum = m_labels[0] + m_labels[1] + m_labels[2];
    bool conflict = true;
    for (size_t k = 0; k <= m_number_of_labels - 2; k += 2) {
        if (part_sum == sum) {
            conflict = false;
            break;
        }
        part_sum = part_sum + m_labels[(3 + k) % m_number_of_labels] +
                   m_labels[(4 + k) % m_number_of_labels];
        part_sum = part_sum - m_labels[k] - m_labels[(1 + k) % m_number_of_labels];
    }

    for (const size_t att_idx : pos_2) {
        m_labels[static_cast<size_t>(m_cycle_labels[att_idx])] = 0;
    }

    return conflict;
}

void compute_conflicts(
    const std::vector<Segment>& segments, const Cycle& cycle, Graph& interlacement_graph
) {
    if (segments.size() <= 1)
        return;
    for (size_t i = 0; i < segments.size() - 1; ++i) {
        CycleConflictDetector detector(cycle.size(), segments[i].get_attachments());
        for (size_t j = i + 1; j < segments.size(); ++j) {
            if (detector.in_conflict(segments[j].get_attachments()))
                interlacement_graph.add_edge(i, j);
        }
    }
}

Graph compute_interlacement_graph(const std::vector<Segment>& segments, const Cycle& cycle) {
    Graph interlacement_graph;
    for (size_t i = 0; i < segments.size(); ++i)
        interlacement_graph.add_node();
    compute_conflicts(segments, cycle, interlacement_graph);
    return interlacement_graph;
}

} // namespace domus::planarity