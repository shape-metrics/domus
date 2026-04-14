#include "splitting_paths.hpp"

namespace domus::torus {
using namespace domus::graph;

Path path_of_chord(const Graph& graph, const Bridge& chord) {
    DOMUS_ASSERT(
        chord.get_bridge().get_number_of_nodes() == 2,
        "path_of_chord: function only for bridges that are a single edge"
    );
    const size_t node_1 = chord.get_new_id_to_old_id().get_label(0);
    const size_t edge_id = chord.get_new_edge_id_to_old_id().get_label(0);
    Path path;
    path.push_back(graph, node_1, edge_id);
    return path;
}

} // namespace domus::torus
