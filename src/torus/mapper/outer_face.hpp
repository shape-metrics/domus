#pragma once

#include <bitset>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/graph/path.hpp"

namespace domus::torus::mapper {

class OuterFace {
    graph::Path m_path;
    std::array<graph::Path, 3> m_repeated_paths;
    graph::utilities::NodesLabels<std::bitset<3>> m_is_node_in_repeated_path;

  public:
    OuterFace(
        const graph::Graph& graph, graph::Path&& path, std::array<graph::Path, 3>&& repeated_paths
    );

    const graph::Path& path() const;
    const std::array<graph::Path, 3>& repeated_paths() const;
    const graph::utilities::NodesLabels<std::bitset<3>>& is_node_in_repeated_path() const;
    std::string to_string() const;
    void print() const;
};

OuterFace compute_outer_face(const graph::Graph& graph, const graph::Embedding& embedding);

} // namespace domus::torus::mapper