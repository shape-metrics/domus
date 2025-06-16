#ifndef MY_AREA_COMPACTER_HPP
#define MY_AREA_COMPACTER_HPP

#include "core/graph/attributes.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/drawing_builder.hpp"
#include "orthogonal/shape/shape.hpp"

void compact_area(const Graph& graph, const Shape& shape,
                  GraphAttributes& attributes);

#endif