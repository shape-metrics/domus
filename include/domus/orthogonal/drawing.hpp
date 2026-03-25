#pragma once

#include <expected>
#include <filesystem>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/shape/shape.hpp"

namespace domus::orthogonal {

struct OrthogonalDrawing {
    graph::Graph augmented_graph;
    graph::Attributes attributes;
    shape::Shape shape;
};

std::expected<void, std::string> make_svg(
    const graph::Graph& graph, const graph::Attributes& attributes, std::filesystem::path path
);

std::expected<void, std::string>
save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, std::filesystem::path path);

std::expected<OrthogonalDrawing, std::string>
load_orthogonal_drawing_from_file(std::filesystem::path path);

std::pair<std::vector<size_t>, std::vector<size_t>>
compute_node_to_index_position(const graph::Graph& graph, const graph::Attributes& attributes);

} // namespace domus::orthogonal