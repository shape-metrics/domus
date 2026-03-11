#pragma once

#include <deque>
#include <string>
#include <vector>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Cycle;

struct Segment {
  private:
    Graph segment;
    NodesContainer attachments;

  public:
    Graph& get_segment();
    const Graph& get_segment() const;
    const NodesContainer& get_attachments() const;
    bool has_attachment(size_t attachment_id) const;
    void add_attachment(size_t attachment_id);
    std::string to_string() const;
    void print() const;
    Segment();
};

std::vector<Segment> compute_segments(const Graph& graph, const Cycle& cycle);

bool is_segment_a_path(const Segment& segment);

std::deque<size_t>
compute_path_between_attachments(const Segment& segment, size_t attachment_1, size_t attachment_2);