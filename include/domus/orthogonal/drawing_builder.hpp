#ifndef MY_DRAWING_BUILDER_H
#define MY_DRAWING_BUILDER_H

#include <expected>
#include <filesystem>
#include <stddef.h>
#include <string>

#include "domus/orthogonal/drawing.hpp"

class UndirectedGraph;

struct ShapeMetricsDrawing {
    OrthogonalDrawing drawing;
    size_t initial_number_of_cycles;
    size_t number_of_added_cycles;
    size_t number_of_useless_bends;
};

std::expected<ShapeMetricsDrawing, std::string>
make_orthogonal_drawing(const UndirectedGraph& graph);

void save_shape_metrics_drawing_to_file(
    const ShapeMetricsDrawing& result, std::filesystem::path path
);

ShapeMetricsDrawing load_shape_metrics_drawing_from_file(std::filesystem::path path);

#endif
