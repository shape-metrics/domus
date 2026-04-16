#pragma once

#include <vector>

#include "domus/core/graph/path.hpp"

namespace domus::graph {
class Path;
}

namespace domus::torus {

enum class FaceType {
    TYPE_1 = 1,
    TYPE_2 = 2,
    TYPE_3 = 3,
    TYPE_4 = 4,
};

class Face {
    FaceType m_type;
    graph::Path m_path;
    std::vector<graph::Path> m_repeated_paths;

  public:
    Face(FaceType type, graph::Path&& path, std::vector<graph::Path>&& repeated_paths);

    FaceType type() const;
    const graph::Path& path() const;
    const std::vector<graph::Path>& repeated_paths() const;
    std::string to_string() const;
    void print() const;
};

size_t node_id_count_in_path(const graph::Path& path, size_t node_id);

Face compute_face_from_path(const graph::Path& path, const graph::Graph& graph);

} // namespace domus::torus