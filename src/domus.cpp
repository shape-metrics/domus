#include <filesystem>
#include <string>

#include "domus/core/graph/file_loader.hpp"
#include "domus/torus/test.hpp"

using namespace domus;
using namespace domus::graph;
using namespace domus::torus;

namespace GraphType = domus::torus::test::GraphType;

int main(int argc, char* argv[]) {
    if (argc < 3)
        return 0;

    std::filesystem::path dataset = argv[1];
    std::filesystem::path obstructions_directory = argv[2];

    const auto graphs = loader::load_graphs_from_asc_file(dataset).value();

    const auto result = torus::test::find_minimal_obstructions(graphs);

    if (std::filesystem::exists(obstructions_directory))
        std::filesystem::remove_all(obstructions_directory);
    std::filesystem::create_directory(obstructions_directory);
    std::filesystem::path non_toroidals = obstructions_directory / "non-toroidal";
    std::filesystem::create_directory(non_toroidals);
    std::filesystem::path minimals = obstructions_directory / "minimal";
    std::filesystem::create_directory(minimals);
    std::filesystem::path planars = obstructions_directory / "planar";
    std::filesystem::create_directory(planars);
    std::filesystem::path toroidals = obstructions_directory / "toroidal";
    std::filesystem::create_directory(toroidals);
    std::filesystem::path non_biconnected = obstructions_directory / "non-biconnected";
    std::filesystem::create_directory(non_biconnected);
    std::filesystem::path uncomputed = obstructions_directory / "uncomputed";
    std::filesystem::create_directory(uncomputed);

    for (size_t i = 0; i < result.size(); i++) {
        std::filesystem::path path;
        if (std::holds_alternative<GraphType::MinimalNonToroidal>(result[i])) {
            path = minimals / (std::to_string(i) + ".txt");
            if (domus::torus::test::is_toroidal_ground_truth(graphs[i]))
                std::print("Error: {} labeled minimal non-toroidal but is toroidal\n", i);
        } else if (std::holds_alternative<GraphType::NonToroidal>(result[i])) {
            path = non_toroidals / (std::to_string(i) + ".txt");
            if (domus::torus::test::is_toroidal_ground_truth(graphs[i]))
                std::print("Error: {} labeled non-toroidal but is toroidal\n", i);
        } else if (std::holds_alternative<GraphType::Planar>(result[i])) {
            const auto embedding = std::get<GraphType::Planar>(result[i]).embedding;
            path = planars / (std::to_string(i) + ".txt");
            loader::save_embedding_to_file(embedding, path).value();
            continue;
        } else if (std::holds_alternative<GraphType::Toroidal>(result[i])) {
            const auto embedding = std::get<GraphType::Toroidal>(result[i]).embedding;
            path = toroidals / (std::to_string(i) + ".txt");
            loader::save_embedding_to_file(embedding, path).value();
            continue;
        } else if (std::holds_alternative<GraphType::NonBiconnected>(result[i])) {
            path = obstructions_directory / "non-biconnected" / (std::to_string(i) + ".txt");
        } else if (std::holds_alternative<GraphType::Uncomputed>(result[i])) {
            path = obstructions_directory / "uncomputed" / (std::to_string(i) + ".txt");
        }
        loader::save_graph_to_file(graphs[i], path).value();
    }

    return 0;
}
