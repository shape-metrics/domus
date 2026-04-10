#pragma once

#include <utility>

#include "faces.hpp"

namespace domus::graph {
class Graph;
class Path;
} // namespace domus::graph

namespace domus::torus {

class NextTypesEstablisher {
    const graph::Graph& m_graph;
    const graph::Path& m_splitting_path;
    const Face& m_old_type_4_face;
    const graph::Path& m_face_1;
    const graph::Path& m_face_2;

    NextTypesEstablisher(
        const graph::Graph& graph,
        const graph::Path& splitting_path,
        const Face& old_type_4_face,
        const graph::Path& face_1,
        const graph::Path& face_2
    );

    std::vector<graph::Path> repeated_paths_both_common_endpoints(const graph::Path& type_1_face);
    graph::Path repeated_path_one_common_endpoint_loop(const graph::Path& type_1_face);
    std::vector<graph::Path> repeated_paths_double_cylinder();
    graph::Path repeated_sub_path(
        const graph::Path& type_1_face,
        const graph::Path& repeated_path,
        const size_t in_between_node_id
    );
    graph::Path repeated_path_one_common_endpoint_no_loop_type_2(const graph::Path& type_1_face);
    std::vector<graph::Path>
    repeated_paths_one_common_endpoint_no_loop_type_3(const graph::Path& type_1_face);
    std::vector<graph::Path>
    repeated_paths_no_common_endpoint_type_3(const graph::Path& type_1_face);
    std::pair<Face, Face> compute_face_types();

  public:
    static std::pair<Face, Face> compute_face_types(
        const graph::Graph& graph,
        const graph::Path& splitting_path,
        const Face& old_type_4_face,
        const graph::Path& face_1,
        const graph::Path& face_2
    );
};

} // namespace domus::torus
