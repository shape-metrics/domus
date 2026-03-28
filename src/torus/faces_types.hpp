#pragma once

#include "domus/core/graph/path.hpp"

namespace domus::torus {

enum class FaceType {
    TYPE_1,
    TYPE_2,
    TYPE_3,
    TYPE_4,
};

FaceType compute_type_of_face(const graph::Path& path);

} // namespace domus::torus