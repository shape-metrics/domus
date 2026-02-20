#ifndef MY_GRAPH_FILE_LOADER_H
#define MY_GRAPH_FILE_LOADER_H

#include <expected>
#include <filesystem>
#include <string>

#include "domus/core/graph/graph.hpp"

class GraphAttributes;

std::expected<UndirectedGraph, std::string> load_graph_from_txt_file(std::filesystem::path path);

std::expected<void, std::string>
save_graph_to_file(const UndirectedGraph& graph, std::filesystem::path path);

std::expected<void, std::string> save_graph_to_graphml_file(
    const UndirectedGraph& graph, const GraphAttributes& attributes, std::filesystem::path path
);

#endif