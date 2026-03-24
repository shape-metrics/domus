#pragma once

#include <expected>
#include <filesystem>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/shape/shape.hpp"

namespace domus::orthogonal {
using Graph = domus::graph::Graph;
using GraphAttributes = domus::graph::GraphAttributes;
using Shape = domus::orthogonal::shape::Shape;

struct OrthogonalDrawing {
    Graph augmented_graph;
    GraphAttributes attributes;
    Shape shape;
};

std::expected<void, std::string>
make_svg(const Graph& graph, const GraphAttributes& attributes, std::filesystem::path path);

std::expected<void, std::string>
save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, std::filesystem::path path);

std::expected<OrthogonalDrawing, std::string>
load_orthogonal_drawing_from_file(std::filesystem::path path);

std::pair<std::vector<size_t>, std::vector<size_t>>
compute_node_to_index_position(const Graph& graph, const GraphAttributes& attributes);

} // namespace domus::orthogonal