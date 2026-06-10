#pragma once

#include "domus/core/graph/embedding.hpp"
#include "domus/torus/faces.hpp"

namespace domus::torus {

void draw_face_with_path(
    const graph::Embedding& embedding, const Face& original_face, const graph::Path& path
);

void draw_face_with_2_paths_path(
    const graph::Embedding& embedding,
    const Face& original_face,
    const graph::Path& path_1,
    const graph::Path& path_2
);

} // namespace domus::torus