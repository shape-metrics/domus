#include "faces.hpp"

#include "domus/core/graph/path.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <print>
#include <vector>

namespace domus::torus {
using graph::Path;

std::string face_type_to_string(FaceType face_type) {
    switch (face_type) {
    case FaceType::TYPE_1:
        return "Type 1";
    case FaceType::TYPE_2:
        return "Type 2";
    case FaceType::TYPE_3:
        return "Type 3";
    case FaceType::TYPE_4:
        return "Type 4";
    }
}

Face::Face(FaceType type, Path&& path, std::vector<Path>&& repeated_paths)
    : m_type(type), m_path(path), m_repeated_paths(repeated_paths) {}

FaceType Face::type() const { return m_type; }

const Path& Face::path() const { return m_path; }

const std::vector<Path>& Face::repeated_paths() const { return m_repeated_paths; }

std::optional<size_t> is_face_simple(const Path& path) {
    std::vector<size_t> nodes_in_path;
    nodes_in_path.reserve(path.number_of_edges() + 1);
    for (auto [edge_id, prev_node_id] : path.get_edges())
        nodes_in_path.push_back(prev_node_id);

    std::sort(nodes_in_path.begin(), nodes_in_path.end());

    for (size_t i = 0; i < nodes_in_path.size() - 1; ++i)
        if (nodes_in_path[i] == nodes_in_path[i + 1])
            return nodes_in_path[i];

    return std::nullopt;
}

std::string Face::to_string() const {
    std::string result;
    auto out = std::back_inserter(result);
    std::format_to(out, "Face\n");
    std::format_to(out, "type: {}\n", face_type_to_string(type()));
    std::format_to(out, "{}", path().to_string());
    std::format_to(out, "Repeated paths:\n");
    for (const Path& path : repeated_paths())
        std::format_to(out, "{}", path.to_string());
    return result;
}

void Face::print() const { std::print("{}", to_string()); }

size_t node_id_count_in_path(const graph::Path& path, const size_t node_id) {
    size_t count = 0;
    for (size_t i = 0; i < path.number_of_edges(); ++i)
        if (path.node_id_at_position(i) == node_id)
            count++;
    return count;
}

} // namespace domus::torus