#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "domus/core/graph/graph.hpp"

namespace domus::graph {
class Attributes;
}

namespace domus::graph::loader {

std::expected<Graph, std::string> load_graph_from_txt_file(std::filesystem::path path);

std::expected<void, std::string> save_graph_to_file(const Graph& graph, std::filesystem::path path);

std::expected<void, std::string> save_graph_to_graphml_file(
    const Graph& graph, const Attributes& attributes, std::filesystem::path path
);

} // namespace domus::graph::loader