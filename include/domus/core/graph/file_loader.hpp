#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "domus/core/graph/graph.hpp"

class GraphAttributes;

std::expected<Graph, std::string> load_graph_from_txt_file(std::filesystem::path path);

std::expected<void, std::string> save_graph_to_file(const Graph& graph, std::filesystem::path path);

std::expected<void, std::string> save_graph_to_graphml_file(
    const Graph& graph, const GraphAttributes& attributes, std::filesystem::path path
);