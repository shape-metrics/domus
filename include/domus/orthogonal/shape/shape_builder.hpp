#pragma once

#include <vector>

#include "domus/orthogonal/shape/shape.hpp"

namespace domus::graph {
class Cycle;
class Attributes;
class Graph;
} // namespace domus::graph

namespace domus::orthogonal::shape {

Shape build_shape(
    graph::Graph& graph,
    graph::Attributes& attributes,
    std::vector<graph::Cycle>& cycles,
    bool randomize = false
);
} // namespace domus::orthogonal::shape