#ifndef MY_SHAPE_FILE_LOADER_H
#define MY_SHAPE_FILE_LOADER_H

#include <string>

#include "core/graph/graph.hpp"
#include "orthogonal/shape/shape.hpp"

Shape load_shape_from_file(const std::string& filename);

void save_shape_to_file(const Graph& graph, const Shape& shape,
                        const std::string& filename);

#endif