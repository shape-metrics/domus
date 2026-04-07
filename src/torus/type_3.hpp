#pragma once

namespace domus::graph {
class Graph;
class Embedding;
class Path;
} // namespace domus::graph

namespace domus::torus {

class Face;
enum class FaceType;

void handle_type_3(
    graph::Graph& graph,
    graph::Embedding& embedding,
    const graph::Path& face_1,
    const graph::Path& face_2,
    FaceType face_1_type,
    FaceType face_2_type
);

} // namespace domus::torus
