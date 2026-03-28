#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"

namespace domus::graph {
class Cycle;
}

namespace domus::planarity {
using namespace domus::graph;

class Segment {
  private:
    const Graph m_segment;
    utilities::NodesLabels m_new_id_to_old_id;
    utilities::NodesContainer m_is_attachment;
    utilities::EdgesLabels m_new_edge_id_to_old_id;
    size_t m_number_of_attachments = 0;
    size_t m_cycle_size;

    Segment(
        const Graph&& segment,
        const utilities::NodesLabels&& labels,
        const utilities::EdgesLabels&& edges_labels,
        const size_t cycle_size
    );
    void add_attachment(size_t attachment_id);
    static Segment build_chord(
        const size_t attachment_1,
        const size_t attachment_2,
        const size_t edge_id,
        const Cycle& cycle,
        utilities::NodesLabels& old_id_to_new_id
    );
    static Segment build_segment(
        const std::vector<size_t>& nodes,
        std::vector<graph::EdgeId>& edges,
        const Cycle& cycle,
        utilities::NodesLabels& old_id_to_new_id
    );
    static void find_chords(
        const Graph& graph,
        const Cycle& cycle,
        std::vector<Segment>& segments,
        utilities::NodesLabels& old_id_to_new_id
    );
    static void find_segments(
        const Graph& graph,
        const Cycle& cycle,
        std::vector<Segment>& segments,
        utilities::NodesLabels& old_id_to_new_id
    );

  public:
    const Graph& get_segment() const;
    size_t get_cycle_size() const;
    const utilities::NodesLabels& get_new_id_to_old_id() const;
    const utilities::EdgesLabels& get_new_edge_id_to_old_id() const;
    size_t number_of_attachments() const;
    bool is_attachment(size_t node_id) const;
    std::string to_string() const;
    void print() const;
    static std::vector<Segment> compute(const Graph& graph, const Cycle& cycle);
};

bool is_segment_a_path(const Segment& segment);

Path compute_path_between_attachments(
    const Segment& segment, size_t attachment_1, size_t attachment_2
);

} // namespace domus::planarity