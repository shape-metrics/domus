#include "domus/planarity/interlacement.hpp"

#include <cassert>
#include <stddef.h>
#include <unordered_map>
#include <unordered_set>

#include "domus/core/graph/cycle.hpp"
#include "domus/core/graph/segment.hpp"

using namespace std;

unordered_map<int, int> compute_cycle_labels(const Segment& segment, const Cycle& cycle) {
    unordered_map<int, int> cycle_labels;
    int found_attachments = 0;
    const int total_attachments = static_cast<int>(segment.get_attachments().size());
    for (int node_id : cycle) {
        if (segment.has_attachment(node_id))
            cycle_labels[node_id] = 2 * (found_attachments++);
        else if (found_attachments == 0)
            cycle_labels[node_id] = 2 * total_attachments - 1;
        else
            cycle_labels[node_id] = 2 * found_attachments - 1;
    }
    return cycle_labels;
}

void compute_conflicts(
    const vector<Segment>& segments, const Cycle& cycle, UndirectedGraph& interlacement_graph
) {
    if (segments.size() <= 1)
        return;
    for (size_t i = 0; i < segments.size() - 1; ++i) {
        const Segment& segment = segments[i];
        unordered_map<int, int> cycle_labels = compute_cycle_labels(segment, cycle);
        const size_t number_of_labels = 2 * segment.get_attachments().size();
        vector<int> labels(number_of_labels);
        for (size_t j = i + 1; j < segments.size(); ++j) {
            const Segment& otherSegment = segments[j];
            for (size_t k = 0; k < number_of_labels; ++k)
                labels[k] = 0;
            for (const int attachment_id : otherSegment.get_attachments()) {
                const int cycle_label = cycle_labels[attachment_id];
                assert(cycle_label >= 0);
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
                interlacement_graph.add_edge(static_cast<int>(i), static_cast<int>(j));
        }
    }
}

UndirectedGraph compute_interlacement_graph(const vector<Segment>& segments, const Cycle& cycle) {
    UndirectedGraph interlacement_graph;
    for (int i = 0; i < static_cast<int>(segments.size()); ++i)
        interlacement_graph.add_node(i);
    compute_conflicts(segments, cycle, interlacement_graph);
    return interlacement_graph;
}