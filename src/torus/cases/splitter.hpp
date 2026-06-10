#pragma once

#include <functional>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/torus/bridge.hpp"
#include "domus/torus/faces.hpp"

#include "insertions.hpp"

namespace domus::torus {

class SplitterWithPath {
    graph::Graph& m_graph;
    graph::Embedding& m_embedding;
    const Face& m_face;
    const size_t m_jolly_id;
    const std::vector<Bridge>& m_bridges;
    std::function<std::vector<PathInsertions>(const graph::Path&)> m_compute_path_insertions;

    std::vector<size_t> compute_attachments();
    bool try_embedding_extension(const graph::Path& path);

  public:
    SplitterWithPath(
        graph::Graph& graph,
        graph::Embedding& embedding,
        const Face& face,
        size_t jolly_id,
        const std::vector<Bridge>& bridges,
        std::function<std::vector<PathInsertions>(const graph::Path&)> compute_path_insertions
    );
    bool try_face_splits_with_path(const graph::Path& path);
    bool try_paths_inside_graph();
    bool try_edges_not_in_graph();
};

} // namespace domus::torus