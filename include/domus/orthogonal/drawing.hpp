#ifndef MY_DRAWING_HPP
#define MY_DRAWING_HPP

#include <string>
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

void make_svg(
    const UndirectedGraph& graph, const GraphAttributes& attributes, const std::string& filename
);

void save_orthogonal_drawing_to_file(const OrthogonalDrawing& result, const std::string& path);

OrthogonalDrawing load_orthogonal_drawing_from_file(const std::string& path);

std::pair<std::unordered_map<int, int>, std::unordered_map<int, int>>
compute_node_to_index_position(const UndirectedGraph& graph, const GraphAttributes& attributes);

#endif