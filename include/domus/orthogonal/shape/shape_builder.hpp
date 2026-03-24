#pragma once

#include <vector>

#include "domus/orthogonal/shape/shape.hpp"

namespace domus::graph {
class Cycle;
class GraphAttributes;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal::shape {
using Graph = domus::graph::Graph;
using Cycle = domus::graph::Cycle;
using GraphAttributes = domus::graph::GraphAttributes;

Shape build_shape(
    Graph& graph, GraphAttributes& attributes, std::vector<Cycle>& cycles, bool randomize = false
);
} // namespace domus::orthogonal::shape