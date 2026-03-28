#include "interlacement.hpp"

#include <stddef.h>

#include "domus/core/graph/cycle.hpp"
#include "segment.hpp"

#include "../core/domus_debug.hpp"

namespace domus::planarity {
using Cycle = domus::graph::Cycle;

std::vector<int> compute_cycle_labels(const Segment& segment, const Cycle& cycle) {
    std::vector<int> cycle_labels(cycle.size());
    int found_attachments = 0;
    const int total_attachments = static_cast<int>(segment.number_of_attachments());
    for (size_t i = 0; i < cycle.size(); ++i) {
        if (segment.is_attachment(i))
            cycle_labels[i] = 2 * (found_attachments++);
        else if (found_attachments == 0)
            cycle_labels[i] = 2 * total_attachments - 1;
        else
            cycle_labels[i] = 2 * found_attachments - 1;
    }
    return cycle_labels;
}

void compute_conflicts(
    const std::vector<Segment>& segments, const Cycle& cycle, Graph& interlacement_graph
) {
    if (segments.size() <= 1)
        return;
    for (size_t i = 0; i < segments.size() - 1; ++i) {
        const Segment& segment = segments[i];
        std::vector<int> cycle_labels = compute_cycle_labels(segment, cycle);
        const size_t number_of_labels = 2 * segment.number_of_attachments();
        std::vector<int> labels(number_of_labels);
        for (size_t j = i + 1; j < segments.size(); ++j) {
            const Segment& other_segment = segments[j];
            for (size_t k = 0; k < number_of_labels; ++k)
                labels[k] = 0;
            for (size_t k = 0; k < cycle.size(); ++k) {
                if (!other_segment.is_attachment(k))
                    continue;
                const int cycle_label = cycle_labels[k];
                DOMUS_ASSERT(cycle_label >= 0, "compute_conflicts: internal errors");
                labels[static_cast<size_t>(cycle_label)] = 1;
            }
            int sum = 0;
            for (size_t k = 0; k < number_of_labels; ++k)
                sum += labels[k];
            int part_sum = labels[0] + labels[1] + labels[2];
            bool are_in_conflict = true;
            for (size_t k = 0; k <= number_of_labels - 2; k += 2) {
                if (part_sum == sum) {
                    are_in_conflict = false;
                    break;
                }
                part_sum = part_sum + labels[(3 + k) % number_of_labels] +
                           labels[(4 + k) % number_of_labels];
                part_sum = part_sum - labels[k] - labels[(1 + k) % number_of_labels];
            }
            if (are_in_conflict)
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