#ifndef MY_SEGMENT_HPP
#define MY_SEGMENT_HPP

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

#include "domus/core/graph/graph.hpp"

class Cycle;

struct Segment {
  private:
    UndirectedGraph segment;
    std::unordered_set<int> attachments;

  public:
    UndirectedGraph& get_segment();
    const UndirectedGraph& get_segment() const;
    const std::unordered_set<int>& get_attachments() const;
    bool has_attachment(int attachment_id) const;
    void add_attachment(int attachment_id);
    std::string to_string() const;
    void print() const;
    Segment();
};

std::vector<Segment> compute_segments(const UndirectedGraph& graph, const Cycle& cycle);

bool is_segment_a_path(const Segment& segment);

std::deque<int>
compute_path_between_attachments(const Segment& segment, int attachment_1, int attachment_2);

#endif