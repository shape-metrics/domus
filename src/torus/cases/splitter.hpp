#pragma once

#include <bitset>
#include <functional>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/torus/bridge.hpp"

#include "../faces.hpp"

namespace domus::torus {

enum class InsertionType { AFTER, BEFORE };

struct Insertion {
    const size_t node_id;
    const InsertionType type;
    const size_t edge_id;
    const size_t edge_id_to_insert;
};

struct PathInsertions {
    const Insertion head_insertion;
    const Insertion tail_insertion;
};

class SplitterWithPath {
    graph::Graph& m_graph;
    graph::Embedding& m_embedding;
    const Face& m_face;
    const size_t m_jolly_id;
    const std::vector<Bridge>& m_bridges;
    const graph::utilities::NodesLabels<std::bitset<3>>& m_is_node_in_repeated_path;
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
        const graph::utilities::NodesLabels<std::bitset<3>>& is_node_in_repeated_path,
        std::function<std::vector<PathInsertions>(const graph::Path&)> compute_path_insertions
    );
    bool try_face_splits_with_path(const graph::Path& path);
    bool try_paths_inside_graph();
    bool try_edges_not_in_graph();
};

} // namespace domus::torus