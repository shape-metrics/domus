#pragma once

namespace domus::graph {
class Graph;
class Embedding;
} // namespace domus::graph

namespace domus::torus {
class Face;

void decrease_face_type(graph::Graph& graph, graph::Embedding& embedding, const Face& face);

} // namespace domus::torus
