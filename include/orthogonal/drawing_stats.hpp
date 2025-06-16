#ifndef MY_DRAWING_STATS_H
#define MY_DRAWING_STATS_H

#include "core/graph/graph.hpp"
#include "orthogonal/drawing_builder.hpp"

int compute_total_edge_length(const DrawingResult& result);

int compute_max_edge_length(const DrawingResult& result);

double compute_edge_length_std_dev(const DrawingResult& result);

int compute_total_bends(const DrawingResult& result);

int compute_max_bends_per_edge(const DrawingResult& result);

double compute_bends_std_dev(const DrawingResult& result);

int compute_total_area(const DrawingResult& result);

int compute_total_crossings(const DrawingResult& result);

struct OrthogonalStats {
  int crossings;
  int bends;
  int area;
  int total_edge_length;
  int max_edge_length;
  double edge_length_stddev;
  int max_bends_per_edge;
  double bends_stddev;
};

OrthogonalStats compute_all_orthogonal_stats(const DrawingResult& result);

#endif
