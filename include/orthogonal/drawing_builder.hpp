#ifndef MY_DRAWING_BUILDER_H
#define MY_DRAWING_BUILDER_H

#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "core/graph/attributes.hpp"
#include "core/graph/graph.hpp"
#include "core/utils.hpp"
#include "drawing/linear_scale.hpp"
#include "drawing/svg_drawer.hpp"
#include "orthogonal/shape/shape.hpp"
#include "orthogonal/shape/shape_builder.hpp"

void make_svg(const Graph& graph, const GraphAttributes& attributes,
              const std::string& filename);

struct DrawingResult {
  std::unique_ptr<Graph> augmented_graph;
  GraphAttributes attributes;
  Shape shape;
  int initial_number_of_cycles;
  int number_of_added_cycles;
  int number_of_useless_bends;
};

DrawingResult make_orthogonal_drawing(const Graph& graph);

std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
compute_node_to_index_position(const Graph& graph,
                               const GraphAttributes& attributes);
#endif
