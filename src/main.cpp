#include <iostream>

#include "config/config.hpp"
#include "core/graph/file_loader.hpp"
#include "core/graph/graph.hpp"
#include "orthogonal/drawing_builder.hpp"
#include "orthogonal/drawing_stats.hpp"

int main() {
  Config config("config.txt");
  const std::string& filename = config.get("output_svg");
  auto graph = load_graph_from_txt_file(config.get("input_graph_file"));
  auto result = make_orthogonal_drawing(*graph);
  make_svg(*result.augmented_graph, result.attributes, filename);
  auto stats = compute_all_orthogonal_stats(result);
  std::cout << "Area: " << stats.area << "\n";
  std::cout << "Crossings: " << stats.crossings << "\n";
  std::cout << "Bends: " << stats.bends << "\n";
  std::cout << "Total edge length: " << stats.total_edge_length << "\n";
  std::cout << "Max edge length: " << stats.max_edge_length << "\n";
  std::cout << "Edge length stddev: " << stats.edge_length_stddev << "\n";
  std::cout << "Max bends per edge: " << stats.max_bends_per_edge << "\n";
  std::cout << "Bends stddev: " << stats.bends_stddev << "\n";
  std::cout << "Initial number of cycles: " << result.initial_number_of_cycles
            << "\n";
  std::cout << "Number of added cycles: " << result.number_of_added_cycles
            << "\n";
  std::cout << "Number of useless bends: " << result.number_of_useless_bends
            << "\n";
  return 0;
}