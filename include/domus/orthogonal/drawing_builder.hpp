#pragma once

#include "domus/orthogonal/drawing.hpp"

namespace domus::graph {
class Graph;
}

namespace domus::orthogonal {

struct ShapeMetricsDrawing {
    OrthogonalDrawing drawing;
    size_t initial_number_of_cycles;
    size_t number_of_added_cycles;
    size_t number_of_useless_bends;
};

ShapeMetricsDrawing make_orthogonal_drawing(const graph::Graph& graph);

} // namespace domus::orthogonal