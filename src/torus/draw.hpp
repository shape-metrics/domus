#pragma once

#include "domus/core/graph/embedding.hpp"

#include "faces.hpp"

namespace domus::torus {

void draw_type_4_with_path(
    const graph::Embedding& embedding, const Face& original_face, const graph::Path& path
);

void draw_type_3_with_path(
    const graph::Embedding& embedding, const Face& original_face, const graph::Path& path
);

} // namespace domus::torus