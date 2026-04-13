#pragma once

#include "domus/core/graph/attributes.hpp"
#include <expected>
#include <filesystem>
#include <string>

namespace domus::graph {
class Graph;
class Attributes;

} // namespace domus::graph

namespace domus::planarity {

std::expected<void, std::string> make_svg(
    const graph::Graph& graph, const graph::Attributes& attributes, std::filesystem::path path
);

}
