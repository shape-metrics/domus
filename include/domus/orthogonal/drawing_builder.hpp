#pragma once

#include <expected>
#include <filesystem>
#include <string>

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

std::expected<ShapeMetricsDrawing, std::string> make_orthogonal_drawing(const graph::Graph& graph);

std::expected<void, std::string>
save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result, std::filesystem::path path);

std::expected<ShapeMetricsDrawing, std::string>
load_shape_metrics_drawing_from_file(std::filesystem::path path);

} // namespace domus::orthogonal