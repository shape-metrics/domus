#pragma once

#include <vector>

#include "domus/orthogonal/shape/shape.hpp"

class Cycle;
class GraphAttributes;
class Graph;

Shape build_shape(
    Graph& graph, GraphAttributes& attributes, std::vector<Cycle>& cycles, bool randomize = false
);