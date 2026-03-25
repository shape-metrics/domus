#include "domus/orthogonal/loader.hpp"

#include <fstream>
#include <string>

#include "domus/orthogonal/drawing_builder.hpp"

#include "../nlohmann/json.hpp"

namespace domus::orthogonal::loader {

using json = nlohmann::json;

std::expected<void, std::string>
save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result, std::filesystem::path path) {
    auto saved = save_orthogonal_drawing_to_file(result.drawing, path);
    if (!saved) {
        return std::unexpected(
            std::format("save_shape_metrics_drawing_to_file: {}", saved.error())
        );
    }
    json data;
    data["initial_number_of_cycles"] = result.initial_number_of_cycles;
    data["number_of_added_cycles"] = result.number_of_added_cycles;
    data["number_of_useless_bends"] = result.number_of_useless_bends;
    std::ofstream file(path);
    if (!file.is_open())
        return std::unexpected(
            std::format("save_shape_metrics_drawing_to_file: could not open {}", path.string())
        );
    file << data;
    return {};
}

std::expected<ShapeMetricsDrawing, std::string>
load_shape_metrics_drawing_from_file(std::filesystem::path path) {
    auto drawing = load_orthogonal_drawing_from_file(path);
    if (!drawing) {
        return std::unexpected(
            std::format("load_shape_metrics_drawing_from_file: {}", drawing.error())
        );
    }
    std::ifstream file(path);
    json data;
    file >> data;
    ShapeMetricsDrawing result;
    result.drawing = std::move(*drawing);
    result.initial_number_of_cycles =
        static_cast<size_t>(data.value("initial_number_of_cycles", 0));
    result.number_of_added_cycles = static_cast<size_t>(data.value("number_of_added_cycles", 0));
    result.number_of_useless_bends = static_cast<size_t>(data.value("number_of_useless_bends", 0));
    return result;
}

} // namespace domus::orthogonal::loader