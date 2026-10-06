#include <atomic>
#include <filesystem>
#include <iostream>
#include <string>

#include "domus/core/graph/file_loader.hpp"
#include "domus/torus/test.hpp"

using namespace domus;
using namespace domus::graph;
using namespace domus::torus;

int main(int argc, char* argv[]) {
    if (argc < 2)
        return 0;

    std::filesystem::path dataset_folder = argv[1];
    std::vector<Graph> graphs;
    for (auto& entry : std::filesystem::directory_iterator(dataset_folder))
        graphs.push_back(loader::load_graph_from_txt_file(entry.path()).value());

    std::cout << std::boolalpha << torus::test::compare_with_ground_truth(graphs) << std::endl;

    return 0;
}

// int main(int argc, char* argv[]) {
//     if (argc < 3)
//         return 0;

//     std::filesystem::path dataset = argv[1];
//     std::filesystem::path obstructions_directory = argv[2];

//     const auto graphs = loader::load_graphs_from_asc_file(dataset).value();

//     const auto obstructions = torus::test::find_minimal_obstructions(graphs);

//     std::filesystem::create_directory(obstructions_directory);
//     std::filesystem::path non_toroidals = obstructions_directory / "non-toroidal";
//     std::filesystem::create_directory(non_toroidals);
//     std::filesystem::path minimals = obstructions_directory / "minimal";
//     std::filesystem::create_directory(minimals);

//     for (size_t i = 0; i < obstructions.size(); i++) {
//         const Graph& g = obstructions[i].first;
//         bool is_minimal = obstructions[i].second;
//         if (is_minimal) {
//             std::filesystem::path path = minimals / (std::to_string(i) + ".txt");
//             loader::save_graph_to_file(g, path).value();
//         } else {
//             std::filesystem::path path = non_toroidals / (std::to_string(i) + ".txt");
//             loader::save_graph_to_file(g, path).value();
//         }
//     }

//     return 0;
// }
