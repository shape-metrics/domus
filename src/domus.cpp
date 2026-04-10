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

#include "domus/core/graph/test.hpp"

using namespace domus;
using namespace domus::graph;
using namespace domus::planarity;
using namespace domus::orthogonal;

void planarity_test(const graph::Graph &graph) {
  const std::optional<Embedding> embedding = compute_planar_embedding(graph);
  if (embedding.has_value()) {
    std::println("Embedding found");
    embedding->print();
    std::println("number of faces: {}",
                 compute_number_of_faces_in_embedding(embedding.value()));
  } else
    std::println("Embedding not found");
}

void make_orthogonal(const Graph &graph) {
  static constexpr std::string svg_filename = "drawing.svg";
  const auto result = make_orthogonal_drawing(graph);
  make_svg(result.drawing.augmented_graph, result.drawing.attributes,
           result.drawing.shape, svg_filename)
      .value();
  stats::compute_all_orthogonal_stats(result.drawing).print();
  std::println("Initial number of cycles: {}", result.initial_number_of_cycles);
  std::println("Number of added cycles: {}", result.number_of_added_cycles);
  std::println("Number of useless bends: {}", result.number_of_useless_bends);
}

void toroidal_test(const graph::Graph &graph) {
  const std::optional<Embedding> embedding =
      torus::compute_toroidal_embedding(graph);
  if (embedding.has_value()) {
    std::println("Embedding found");
    embedding->print();
    std::println("number of faces: {}",
                 compute_number_of_faces_in_embedding(embedding.value()));
    std::println("genus: {}", compute_embedding_genus(embedding.value()));
  } else
    std::println("Embedding not found");
}

auto load_graph() {
  std::string input_graph_filename = "graph.txt";
  const auto graph = loader::load_graph_from_txt_file(input_graph_filename);
  if (!graph) {
    println("{}", graph.error());
    std::terminate();
  }
  return graph.value();
}

int main() {
  // graph->print(true);
  // std::println("{}", generators::code_to_generate_graph(*graph));

  // planarity_test(*graph);
  // for (const auto& forbidden_minor : test::forbidden_minors)
  //     planarity_test(forbidden_minor);

  // make_orthogonal(*graph);
  // toroidal_test(test::two_cycle_graphs[2]);

  std::println("k5");
  test::subdivided_k_5.print(true);

  toroidal_test(test::subdivided_k_5);
  return 0;
}
