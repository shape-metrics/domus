#include <filesystem>
#include <iostream>

#include "config/config.hpp"
#include "core/graph/file_loader.hpp"
#include "core/graph/generators.hpp"
#include "core/graph/graph.hpp"

int main() {
  Config config("config.txt");
  const std::string& generated_graphs_folder =
      config.get("generated_graphs_folder");
  if (std::filesystem::exists(generated_graphs_folder)) {
    std::cout << "Folder " << generated_graphs_folder << " already exists."
              << std::endl;
    std::cout << "Do you want to delete it? (y/n): ";
    char answer;
    std::cin >> answer;
    if (answer == 'y' || answer == 'Y') {
      std::filesystem::remove_all(generated_graphs_folder);
    } else {
      std::cout << "Folder not deleted." << std::endl;
      return 0;
    }
  }
  std::filesystem::create_directories(generated_graphs_folder);
  const double min_density = std::stod(config.get("min_graph_density"));
  const double max_density = std::stod(config.get("max_graph_density"));
  const int min_number_of_nodes = std::stoi(config.get("min_number_of_nodes"));
  const int max_number_of_nodes = std::stoi(config.get("max_number_of_nodes"));
  const int graphs_generated_per_same_nodes_number =
      std::stoi(config.get("graphs_generated_per_same_nodes_number"));
  int number_of_generated_graphs = 0;
  for (int number_of_nodes = min_number_of_nodes;
       number_of_nodes <= max_number_of_nodes; ++number_of_nodes) {
    std::string sub_folder =
        generated_graphs_folder + std::to_string(number_of_nodes) + "/";
    std::filesystem::create_directories(sub_folder);
    for (int i = 1; i <= graphs_generated_per_same_nodes_number; ++i) {
      double density = min_density + (max_density - min_density) * i /
                                         graphs_generated_per_same_nodes_number;
      int number_of_edges = static_cast<int>(density * number_of_nodes);
      auto graph = generate_connected_random_graph_degree_max_4(
          number_of_nodes, number_of_edges);
      std::string filename = sub_folder + "graph_" + std::to_string(i) + "_n" +
                             std::to_string(number_of_nodes) + "_m" +
                             std::to_string(number_of_edges) + ".txt";
      save_graph_to_file(*graph, filename);
      ++number_of_generated_graphs;
    }
  }
  std::cout << "Generated " << number_of_generated_graphs << " graphs."
            << std::endl;
  return 0;
}