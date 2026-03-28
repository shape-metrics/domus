#include <optional>
#include <print>
#include <string>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/orthogonal/drawing.hpp"
#include "domus/orthogonal/drawing_builder.hpp"
#include "domus/orthogonal/drawing_stats.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/torus/embedder.hpp"

#include "test_graphs.hpp"

using namespace domus;
using namespace domus::planarity;
using namespace domus::orthogonal;

void planarity_test(const graph::Graph& graph) {
    const std::optional<graph::Embedding> embedding = compute_planar_embedding(graph);
    if (embedding.has_value()) {
        std::println("Embedding found");
        embedding->print();
        std::println(
            "number of faces: {}",
            compute_number_of_faces_in_embedding(embedding.value())
        );
        std::println("{}", is_embedding_planar(graph, embedding.value()));
    } else
        std::println("Embedding not found");
}

void make_orthogonal(const graph::Graph& graph) {
    static constexpr std::string svg_filename = "drawing.svg";
    const auto result = make_orthogonal_drawing(graph);
    make_svg(
        result.drawing.augmented_graph,
        result.drawing.attributes,
        result.drawing.shape,
        svg_filename
    )
        .value();
    stats::compute_all_orthogonal_stats(result.drawing).print();
    std::println("Initial number of cycles: {}", result.initial_number_of_cycles);
    std::println("Number of added cycles: {}", result.number_of_added_cycles);
    std::println("Number of useless bends: {}", result.number_of_useless_bends);
}

void toroidal_test(const graph::Graph& graph) {
    const std::optional<graph::Embedding> embedding = torus::compute_toroidal_embedding(graph);
    if (embedding.has_value()) {
        std::println("Embedding found");
        embedding->print();
        std::println(
            "number of faces: {}",
            compute_number_of_faces_in_embedding(embedding.value())
        );
        std::println("genus: {}", compute_embedding_genus(graph, embedding.value()));
    } else
        std::println("Embedding not found");
}

int main() {
    std::string input_graph_filename = "graph.txt";
    const auto graph = graph::loader::load_graph_from_txt_file(input_graph_filename);
    if (!graph) {
        println("{}", graph.error());
        return 1;
    }
    graph->print(true);
    planarity_test(*graph);
    // make_orthogonal(*graph);
    // toroidal_test(two_cycles_graph_3());
    return 0;
}