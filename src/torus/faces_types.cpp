#include "faces_types.hpp"

namespace domus::torus {

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