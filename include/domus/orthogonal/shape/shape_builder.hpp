#pragma once

#include <vector>

#include "domus/orthogonal/shape/shape.hpp"

class Cycle;
class GraphAttributes;
class UndirectedGraph;

Shape build_shape(
    UndirectedGraph& graph,
    GraphAttributes& attributes,
    std::vector<Cycle>& cycles,
    bool randomize = false
);