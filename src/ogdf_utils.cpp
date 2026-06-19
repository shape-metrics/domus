#include "domus/ogdf_utils.hpp"

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>
#include <ogdf/planarity/BoyerMyrvold.h>

#include "domus/core/domus_debug.hpp"
#include "domus/core/graph/graph.hpp"

namespace domus::ogdf_utils {
using namespace domus::graph;

void find_kuratowski_subdivision(const Graph& graph) {
    ogdf::Graph G;
    std::vector<ogdf::node> nodes;

    for (size_t i = 0; i < graph.get_number_of_nodes(); i++)
        nodes.push_back(G.newNode());

    for (const auto& edge : graph.get_all_edges())
        G.newEdge(nodes[edge.edge.from_id], nodes[edge.edge.to_id], static_cast<int>(edge.id));

    ogdf::BoyerMyrvold bm;
    ogdf::SList<ogdf::KuratowskiWrapper> kuratowski;

    if (bm.planarEmbed(G, kuratowski, 1)) {
        DOMUS_ASSERT(false, "find_kuratowski_subdivision: graph is planar");
    } else {
        for (auto k : kuratowski) {
            std::cout << "Kuratowski subdivision" << std::boolalpha << std::endl;
            std::cout << "is k5: " << k.isK5() << std::endl;
            std::cout << "is k33: " << k.isK33() << std::endl;
        }
    }
}

} // namespace domus::ogdf_utils