#pragma once

#include <vector>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

namespace domus::graph {
class Cycle;
}

namespace domus::planarity {
using Graph = domus::graph::Graph;
using GraphPath = domus::graph::utilities::GraphPath;
using Cycle = domus::graph::Cycle;
using NodesLabels = domus::graph::utilities::NodesLabels;
using NodesContainer = domus::graph::utilities::NodesContainer;
using EdgesLabels = domus::graph::utilities::EdgesLabels;

class Segment {
  private:
    const Graph m_segment;
    std::vector<size_t> m_attachments{};
    NodesLabels m_new_id_to_old_id;
    NodesContainer m_is_attachment;
    EdgesLabels m_edges_labels;

    Segment(const Graph&& segment, const NodesLabels&& labels, const EdgesLabels&& edges_labels);
    void add_attachment(size_t attachment_id);
    static Segment build_chord(
        const size_t attachment_1,
        const size_t attachment_2,
        const size_t edge_id,
        const Cycle& cycle,
        NodesLabels& old_id_to_new_id
    );
    static Segment build_segment(
        const std::vector<size_t>& nodes,
        std::vector<graph::EdgeId>& edges,
        const Cycle& cycle,
        NodesLabels& old_id_to_new_id
    );
    static void find_chords(
        const Graph& graph,
        const Cycle& cycle,
        std::vector<Segment>& segments,
        NodesLabels& old_id_to_new_id
    );
    static void find_segments(
        const Graph& graph,
        const Cycle& cycle,
        std::vector<Segment>& segments,
        NodesLabels& old_id_to_new_id
    );

  public:
    const Graph& get_segment() const;
    const NodesLabels& get_new_id_to_old_id() const;
    const EdgesLabels& get_edge_labels() const;
    void for_each_attachment(std::function<void(size_t)> f) const; // refers to old node_ids
    size_t number_of_attachments() const;
    bool is_attachment(size_t node_id) const;
    std::string to_string() const;
    void print() const;
    static std::vector<Segment> compute(const Graph& graph, const Cycle& cycle);
};

bool is_segment_a_path(const Segment& segment);

GraphPath
compute_path_between_attachments(const Segment& segment, size_t attachment_1, size_t attachment_2);

} // namespace domus::planarity