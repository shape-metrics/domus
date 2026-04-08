#pragma once

namespace domus::graph {
class Graph;
class Embedding;
class Path;
} // namespace domus::graph

namespace domus::torus {

class Face;

void handle_type_3(
    graph::Graph& graph, graph::Embedding& embedding, const Face& face_1, const Face& face_2
);

} // namespace domus::torus
