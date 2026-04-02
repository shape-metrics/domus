#include "faces.hpp"

#include "domus/core/graph/path.hpp"

#include <algorithm>
#include <vector>

namespace domus::torus {

Face::Face(FaceType type, graph::Path&& path, std::vector<graph::Path>&& repeated_paths)
    : m_type(type), m_path(path), m_repeated_paths(repeated_paths) {}

FaceType Face::type() const { return m_type; }
const graph::Path& Face::path() const { return m_path; }
const std::vector<graph::Path>& Face::repeated_paths() const { return m_repeated_paths; }

std::optional<size_t> is_face_simple(const graph::Path& path) {
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

// TODO
// FaceType compute_type_of_face(const graph::Path& path) {}

} // namespace domus::torus