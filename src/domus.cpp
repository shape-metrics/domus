#include <expected>
#include <filesystem>
#include <optional>
#include <print>
#include <string>
#include <vector>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/flow.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/test.hpp"
#include "domus/orthogonal/drawing.hpp"
#include "domus/orthogonal/drawing_builder.hpp"
#include "domus/orthogonal/drawing_stats.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/planarity/tutte.hpp"
#include "domus/torus/embedder.hpp"
#include "domus/torus/test.hpp"

using namespace domus;
using namespace domus::graph;
using namespace domus::planarity;
using namespace domus::orthogonal;
using namespace domus::torus;
using namespace domus::drawing;

void make_orthogonal(const Graph& graph) {
    static constexpr std::string svg_filename = "drawing.svg";
    const auto result = make_orthogonal_drawing(graph);
    make_svg(result.drawing, svg_filename).value();
    stats::compute_all_orthogonal_stats(result.drawing).print();
    std::println("Initial number of cycles: {}", result.initial_number_of_cycles);
    std::println("Number of added cycles: {}", result.number_of_added_cycles);
    std::println("Number of useless bends: {}", result.number_of_useless_bends);
}

bool is_toroidal_hard_check(const Graph& graph) {
    for (const Embedding& embedding : compute_all_possible_embeddings(graph))
        if (compute_embedding_genus(embedding) == 1)
            return true;
    return false;
}

bool is_toroidal(const Graph& graph) {
    const std::optional<Embedding> embedding = torus::compute_toroidal_embedding(graph);
    if (embedding.has_value()) {
        DOMUS_ASSERT(
            compute_embedding_genus(embedding.value()) == 1,
            "toroidal_test: found embedding should have genus 1"
        );
        // EquivalentEmbedding eq_embedding = build_equivalent_embedding(graph, *embedding);
        // eq_embedding.to_torus_mapping().visualize();
        return true;
    }
    return false;
}

auto load_graph() {
    std::string input_graph_filename = "graph.txt";
    const auto graph = loader::load_graph_from_txt_file(input_graph_filename);
    if (!graph) {
        std::println("{}", graph.error());
        std::terminate();
    }
    return graph.value();
}

int main() {
    const auto graphs = loader::load_graphs_from_asc_file("10_3_3.asc");
    if (!graphs) {
        std::println("error: {}", graphs.error());
        return 1;
    }
    for (size_t i = 0; i < graphs->size(); ++i) {
        const Graph& g = (*graphs)[i];
        std::print("{} ", i);
        if (is_graph_planar(g)) {
            std::println("is planar");
            continue;
        }
        bool is_toroidal_smart = is_toroidal(g);
        bool ground_truth = is_toroidal_hard_check(g);
        std::println("{} {}", is_toroidal_smart, ground_truth);
    }

    return 0;
}
