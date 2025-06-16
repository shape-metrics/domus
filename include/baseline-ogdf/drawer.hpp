#ifndef DRAWER_H
#define DRAWER_H

#include <string>

#include "core/graph/graph.hpp"

struct OGDFResult {
  int crossings;
  int bends;
  int area;
  int total_edge_length;
  int max_edge_length;
  double edge_length_stddev;
  int max_bends_per_edge;
  double bends_stddev;
};

OGDFResult create_drawing(const Graph& graph,
                          const std::string svg_output_filename = "",
                          const std::string gml_output_filename = "");

#endif
