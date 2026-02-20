#ifndef MY_DRAWING_HPP
#define MY_DRAWING_HPP

#include <expected>
#include <filesystem>
#include <unordered_map>
#include <utility>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/shape/shape.hpp"

struct OrthogonalDrawing {
    UndirectedGraph augmented_graph;
    GraphAttributes attributes;
    Shape shape;
};

std::expected<void, std::string> make_svg(
    const UndirectedGraph& graph, const GraphAttributes& attributes, std::filesystem::path path
);

std::expected<void, std::string>
save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, std::filesystem::path path);

std::expected<OrthogonalDrawing, std::string>
load_orthogonal_drawing_from_file(std::filesystem::path path);

std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
compute_node_to_index_position(const UndirectedGraph& graph, const GraphAttributes& attributes);

#endif