#ifndef MY_SHAPE_BUILDER_H
#define MY_SHAPE_BUILDER_H

#include <vector>

#include "core/graph/attributes.hpp"
#include "core/graph/cycle.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/shape/shape.hpp"

Shape build_shape(Graph& graph, GraphAttributes& attributes,
                  std::vector<Cycle>& cycles, bool randomize = false);

#endif