// Planarity of a simple subcubic graph G with the rotation (cyclic order of
// incident edges) of two prescribed vertices u and v fixed.

#pragma once

#include <optional>
#include <span>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::torus::frp {

std::optional<graph::Embedding> planar_with_fixed_rotations(
    const graph::Graph& G,
    size_t u,
    std::span<const size_t> rot_u,
    size_t v,
    std::span<const size_t> rot_v
);

} // namespace domus::torus::frp