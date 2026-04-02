#pragma once

#include <vector>

#include "domus/core/graph/path.hpp"

namespace domus::graph {
class Path;
}

namespace domus::torus {

enum class FaceType {
    TYPE_1,
    TYPE_2,
    TYPE_3,
    TYPE_4,
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
};

FaceType compute_type_of_face(const graph::Path& path);

} // namespace domus::torus