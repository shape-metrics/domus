#include "domus/planarity/tutte.hpp"

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/core/graph/path.hpp"

#include "domus/core/domus_debug.hpp"

namespace domus::planarity {
using namespace domus::graph;

void compute_nodes_positions(const Graph& graph, Attributes& attributes, const Path& border) {
    DOMUS_ASSERT(
        attributes.has_attribute(Attribute::NODES_POSITION),
        "compute_nodes_positions: external border is not initialized"
    );

    DOMUS_ASSERT(
        [&]() {
            for (const size_t node_id : graph.get_nodes_ids()) {
                if (!border.contains_node_id(node_id)) {
                    if (attributes.has_position(node_id))
                        return false;
                } else {
                    if (!attributes.has_position(node_id))
                        return false;
                }
            }
            return true;
        }(),
        "compute_nodes_positions: nodes positions not initialized correctly"
    );

    DOMUS_ASSERT(
        algorithms::BiconnectedComponents::compute(graph).get_components().size() == 1,
        "compute_nodes_positions: Tutte algorithm's needs the input graph to be triconnected"
    );
}
} // namespace domus::planarity
