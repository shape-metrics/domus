#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/drawing_builder.hpp"

extern "C" {
int compute_orthogonal_drawing() {
    const UndirectedGraph graph = load_graph_from_txt_file("input.txt");
    if (graph.size() > 30) // graph too large
        return -1;
    try {
        const ShapeMetricsDrawing result = make_orthogonal_drawing(graph);
        make_svg(result.drawing.augmented_graph, result.drawing.attributes, "output.svg");
        save_graph_to_graphml_file(
            result.drawing.augmented_graph,
            result.drawing.attributes,
            "output.graphml"
        );
        return 0;
    } catch (const DisconnectedGraphError& e) { // graph is not connected
        return -2;
    } catch (const std::exception& e) { // other errors
        return -3;
    }
}
}