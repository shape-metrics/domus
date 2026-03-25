#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace domus::orthogonal {
struct ShapeMetricsDrawing;
}

namespace domus::orthogonal::loader {
std::expected<void, std::string>
save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result, std::filesystem::path path);

std::expected<ShapeMetricsDrawing, std::string>
load_shape_metrics_drawing_from_file(std::filesystem::path path);
} // namespace domus::orthogonal::loader