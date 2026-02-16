#include <filesystem>
#include <iostream>
#include <stddef.h>
#include <stdexcept>
#include <string>

#include "domus/config/config.hpp"
#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/generators.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"

using namespace std;

int main() {
    const Config config("config.txt");
    const string generated_graphs_folder = config.get("generated_graphs_folder");
    if (filesystem::exists(generated_graphs_folder)) {
        std::cout << "Folder " << generated_graphs_folder << " already exists.\n";
        std::cout << "Do you want to delete it? (y/n): ";
        char answer;
        std::cin >> answer;
        if (answer == 'y' || answer == 'Y') {
            filesystem::remove_all(generated_graphs_folder);
        } else {
            std::cout << "Folder not deleted.\n";
            return 0;
        }
    }
    filesystem::create_directories(generated_graphs_folder);
    const double min_density = std::stod(config.get("min_graph_density"));
    const double max_density = std::stod(config.get("max_graph_density"));
    const int min_number_of_nodes = std::stoi(config.get("min_number_of_nodes"));
    const int max_number_of_nodes = std::stoi(config.get("max_number_of_nodes"));
    const int graphs_generated_per_same_nodes_number =
        std::stoi(config.get("graphs_generated_per_same_nodes_number"));
    int number_of_generated_graphs = 0;
    for (int number_of_nodes = min_number_of_nodes; number_of_nodes <= max_number_of_nodes;
         ++number_of_nodes) {
        string sub_folder = generated_graphs_folder + std::to_string(number_of_nodes) + "/";
        filesystem::create_directories(sub_folder);
        for (int i = 1; i <= graphs_generated_per_same_nodes_number; ++i) {
            const double density = min_density + (max_density - min_density) * i /
                                                     graphs_generated_per_same_nodes_number;
            const int number_of_edges = static_cast<int>(density * number_of_nodes);
            std::cout << "\rGenerating graph with " << number_of_nodes << " nodes and "
                      << number_of_edges << " edges.     ";
            UndirectedGraph graph = generate_connected_random_graph_degree_max_4(
                static_cast<size_t>(number_of_nodes),
                static_cast<size_t>(number_of_edges)
            );
            string filename = sub_folder + "graph_" + std::to_string(i) + "_n" +
                              std::to_string(number_of_nodes) + "_m" +
                              std::to_string(number_of_edges) + ".txt";
            if (!is_graph_connected(graph))
                throw runtime_error("Generated graph is not connected!");
            save_graph_to_file(graph, filename);
            ++number_of_generated_graphs;
        }
    }
    std::cout << "\nGenerated " << number_of_generated_graphs << " graphs.\n";
    return 0;
}