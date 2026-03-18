#pragma once

#include <deque>
#include <string>
#include <vector>

#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graph_utilities.hpp"

class Cycle;

class Segment {
  private:
    Graph segment;
    std::vector<size_t> attachments;
    NodesLabels new_id_to_old_id;
    NodesContainer m_is_attachment; // refers to new node_ids
    Segment();

  public:
    Graph& get_segment();
    const Graph& get_segment() const;
    void for_each_attachment(std::function<void(size_t)> f) const; // refers to old node_ids
    size_t number_of_attachments() const;
    bool is_attachment(size_t attachment_id) const;
    void add_attachment(size_t attachment_id);
    std::string to_string() const;
    void print() const;
    static std::vector<Segment> compute_segments(const Graph& graph, const Cycle& cycle);
};

bool is_segment_a_path(const Segment& segment);

std::deque<size_t>
compute_path_between_attachments(const Segment& segment, size_t attachment_1, size_t attachment_2);