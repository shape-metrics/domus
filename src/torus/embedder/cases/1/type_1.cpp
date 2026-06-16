#include "type_1.hpp"

#include "../utils.hpp"

namespace domus::torus {
bool handle_type_1(
    graph::Graph& graph, graph::Embedding& embedding, const std::vector<Face>& faces
) {
    add_log_final_configuration(faces);
    // TODO
    return false;
}
} // namespace domus::torus
