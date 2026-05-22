#include "type_2.hpp"

#include "../utils.hpp"

namespace domus::torus {
bool handle_type_2(
    graph::Graph& graph, graph::Embedding& embedding, const std::vector<Face>& faces
) {
    add_log_final_configuration(faces);
    // TODO
}
} // namespace domus::torus
