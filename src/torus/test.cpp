#include "domus/torus/test.hpp"

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <print>
#include <sstream>
#include <string>
#include <thread>

#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/graphs_algorithms.hpp"
#include "domus/ogdf_utils.hpp"
#include "domus/torus/embedder.hpp"

#include "faces.hpp"

namespace domus::torus::test {
using namespace graph;

void test_all_possible_embeddings(const Graph& graph) {
    std::vector<Embedding> embeddings = compute_all_possible_embeddings(graph);
    std::println("Number of embeddings: {}", embeddings.size());
    std::vector<size_t> genuses;
    std::vector<size_t> max_face_types;
    for (const Embedding& embedding : embeddings) {
        const std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
        const size_t g = compute_embedding_genus(
            graph.get_number_of_nodes(),
            graph.get_number_of_edges(),
            faces.size(),
            1
        );
        if (genuses.size() <= g)
            genuses.resize(g + 1);
        ++genuses[g];
        if (g == 1) {
            size_t max_face_type = 0;
            for (FaceType type : std::views::transform(faces, [&](const Path& path) {
                     return compute_face_from_path(Path(path), graph).type();
                 })) {
                if (static_cast<size_t>(type) > max_face_type)
                    max_face_type = static_cast<size_t>(type);
            }

            if (max_face_types.size() <= static_cast<size_t>(max_face_type))
                max_face_types.resize(static_cast<size_t>(max_face_type) + 1);
            ++max_face_types[static_cast<size_t>(max_face_type)];
        }
    }

    for (size_t i = 1; i < genuses.size(); ++i) {
        std::println("Genus: [{:>2}] Quantity: [{:>4}]", i, genuses[i]);
    }

    for (size_t i = 1; i < max_face_types.size(); ++i) {
        std::println("Case: [{:>2}] Quantity: [{:>4}]", i, max_face_types[i]);
    }
}

bool is_toroidal_ground_truth(const Graph& graph) {
    for (const Embedding& embedding : compute_all_possible_embeddings(graph))
        if (compute_embedding_genus(embedding) <= 1)
            return true;
    return false;
}

bool is_minimal_obstruction(const Graph& graph) {
    const size_t m = graph.get_number_of_edges();
    for (size_t i = 0; i < m; ++i) {
        Graph g = graph;
        g.remove_edge(i);
        if (!is_toroidal(g))
            return false;
    }
    return true;
}

std::vector<ObstructionResult> find_minimal_obstructions(const std::vector<Graph>& graphs) {
    const size_t total_graphs = graphs.size();

    std::atomic<size_t> current_idx{0};

    std::atomic<size_t> planar{0};
    std::atomic<size_t> toroidal{0};
    std::atomic<size_t> non_toroidal{0};
    std::atomic<size_t> non_biconnected{0};
    std::atomic<size_t> minimal_obstruction{0};
    std::atomic<size_t> processed{0};

    std::vector<ObstructionResult> non_toroidals(graphs.size(), GraphType::Uncomputed{});
    std::mutex mutex;

    const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
    std::println("Processing {} graphs using {} threads...", total_graphs, num_threads);

    auto worker = [&]() {
        while (true) {
            const size_t idx = current_idx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= total_graphs) {
                break;
            }

            const Graph& g = (graphs)[idx];
            if (!algorithms::is_biconnected(g)) {
                non_biconnected.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard lock(mutex);
                non_toroidals[idx] = GraphType::NonBiconnected{};
            } else if (
                auto planar_emb = ogdf_utils::compute_planar_embedding(g); planar_emb.has_value()
            ) {
                planar.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard lock(mutex);
                non_toroidals[idx] = GraphType::Planar{.embedding = std::move(*planar_emb)};
            } else if (
                auto torus_emb = compute_toroidal_embedding_biconnected(g); torus_emb.has_value()
            ) {
                toroidal.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard lock(mutex);
                non_toroidals[idx] = GraphType::Toroidal{.embedding = std::move(*torus_emb)};
            } else {
                non_toroidal.fetch_add(1, std::memory_order_relaxed);
                if (is_minimal_obstruction(g)) {
                    minimal_obstruction.fetch_add(1, std::memory_order_relaxed);
                    std::lock_guard lock(mutex);
                    non_toroidals[idx] = GraphType::MinimalNonToroidal{};
                } else {
                    std::lock_guard lock(mutex);
                    non_toroidals[idx] = GraphType::NonToroidal{};
                }
            }

            const size_t count = processed.fetch_add(1, std::memory_order_relaxed) + 1;
            if (count % 25 == 0 || count == total_graphs) {
                std::lock_guard lock(mutex);
                std::print(
                    "\r [{}/{}] toroidal [{}] non toroidal [{}] minimal obstruction [{}] planar "
                    "[{}] non biconnected [{}]",
                    count,
                    total_graphs,
                    toroidal.load(std::memory_order_relaxed),
                    non_toroidal.load(std::memory_order_relaxed),
                    minimal_obstruction.load(std::memory_order_relaxed),
                    planar.load(std::memory_order_relaxed),
                    non_biconnected.load(std::memory_order_relaxed)
                );
                std::fflush(stdout);
            }
        }
    };

    std::vector<std::jthread> threads;
    threads.reserve(num_threads);
    for (unsigned int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads)
        if (t.joinable())
            t.join();

    std::println();
    std::println(
        "Finished: toroidal [{}] non toroidal [{}] minimal obstruction [{}] planar [{}] non "
        "biconnected [{}]",
        toroidal.load(),
        non_toroidal.load(),
        minimal_obstruction.load(),
        planar.load(),
        non_biconnected.load()
    );

    return non_toroidals;
}

std::vector<Graph> three_regular_known_obstructions(const std::string& filepath) {
    std::ifstream infile(filepath);
    std::vector<Graph> results;
    std::string line;

    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        size_t n = 0;
        std::string bitstring;

        if (!(iss >> n >> bitstring))
            continue;

        if (n % 2 != 0)
            continue;

        const size_t expected_bits = static_cast<size_t>(n * (n - 1) / 2);
        if (bitstring.size() != expected_bits) {
            std::cerr << "Warning: ignored row. expected bits: " << expected_bits
                      << ", found bits: " << bitstring.size() << std::endl;
            continue;
        }

        std::vector<size_t> deg(n, 0);
        size_t bit_idx = 0;
        for (size_t u = 0; u < n; ++u) {
            for (size_t v = u + 1; v < n; ++v) {
                if (bitstring[bit_idx++] == '1') {
                    deg[u]++;
                    deg[v]++;
                }
            }
        }

        bool regular = true;
        for (size_t d : deg) {
            if (d != 3) {
                regular = false;
                break;
            }
        }

        if (!regular)
            continue;

        Graph graph;
        for (size_t i = 0; i < n; i++)
            graph.add_node();
        bit_idx = 0;
        for (size_t u = 0; u < n; ++u)
            for (size_t v = u + 1; v < n; ++v)
                if (bitstring[bit_idx++] == '1')
                    graph.add_edge(u, v);

        results.push_back(std::move(graph));
    }

    std::cout << "3-regular obstructions: " << results.size() << std::endl;
    std::map<size_t, size_t> n_count;
    for (const Graph& g : results)
        n_count[g.get_number_of_nodes()]++;
    for (const auto& [n, count] : n_count)
        std::println("{} graphs with {} vertices", count, n);

    return results;
}

bool compare_with_ground_truth(const std::vector<Graph>& graphs) {
    const size_t total_graphs = graphs.size();
    if (total_graphs == 0)
        return true;

    const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
    std::atomic<size_t> current_idx{0};
    std::atomic<bool> all_match{true};

    auto worker = [&]() {
        while (all_match.load(std::memory_order_relaxed)) {
            const size_t idx = current_idx.fetch_add(1, std::memory_order_relaxed);
            if (idx >= total_graphs)
                break;

            const Graph& g = graphs[idx];
            if (is_toroidal_biconnected(g) != is_toroidal_ground_truth(g)) {
                all_match.store(false, std::memory_order_relaxed);
                break;
            }
        }
    };

    std::vector<std::jthread> threads;
    threads.reserve(num_threads);
    for (unsigned int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads)
        if (t.joinable())
            t.join();

    return all_match.load();
}

void save_all_results(
    const std::vector<Graph>& graphs,
    const std::vector<ObstructionResult>& results,
    const std::filesystem::path& obstructions_directory,
    bool check_correctness
) {
    static constexpr std::string NON_TOROIDAL = "non-toroidal";
    static constexpr std::string MINIMAL = "minimal";
    static constexpr std::string PLANAR = "planar";
    static constexpr std::string TOROIDAL = "toroidal";
    static constexpr std::string NON_BICONNECTED = "non-biconnected";
    static constexpr std::string UNCOMPUTED = "uncomputed";

    if (std::filesystem::exists(obstructions_directory))
        std::filesystem::remove_all(obstructions_directory);
    std::filesystem::create_directory(obstructions_directory);
    std::filesystem::path non_toroidals = obstructions_directory / NON_TOROIDAL;
    std::filesystem::create_directory(non_toroidals);
    std::filesystem::path minimals = obstructions_directory / MINIMAL;
    std::filesystem::create_directory(minimals);
    std::filesystem::path planars = obstructions_directory / PLANAR;
    std::filesystem::create_directory(planars);
    std::filesystem::path toroidals = obstructions_directory / TOROIDAL;
    std::filesystem::create_directory(toroidals);
    std::filesystem::path non_biconnected = obstructions_directory / NON_BICONNECTED;
    std::filesystem::create_directory(non_biconnected);
    std::filesystem::path uncomputed = obstructions_directory / UNCOMPUTED;
    std::filesystem::create_directory(uncomputed);

    for (size_t i = 0; i < results.size(); i++) {
        std::filesystem::path path;
        if (std::holds_alternative<GraphType::MinimalNonToroidal>(results[i])) {
            path = minimals / (std::to_string(i) + ".txt");
            if (check_correctness && domus::torus::test::is_toroidal_ground_truth(graphs[i])) {
                std::print("Error: {} labeled minimal non-toroidal but is toroidal\n", i);
                path = uncomputed / (std::to_string(i) + ".txt");
            }
            loader::save_graph_to_file(graphs[i], path).value();
            continue;
        }
        if (std::holds_alternative<GraphType::NonToroidal>(results[i])) {
            path = non_toroidals / (std::to_string(i) + ".txt");
            if (check_correctness && domus::torus::test::is_toroidal_ground_truth(graphs[i])) {
                std::print("Error: {} labeled non-toroidal but is toroidal\n", i);
                path = uncomputed / (std::to_string(i) + ".txt");
            }
            loader::save_graph_to_file(graphs[i], path).value();
            continue;
        }
        if (std::holds_alternative<GraphType::Planar>(results[i])) {
            const auto& embedding = std::get<GraphType::Planar>(results[i]).embedding;
            path = planars / (std::to_string(i) + ".txt");
            if (!check_correctness || compute_embedding_genus(embedding) == 0)
                loader::save_embedding_to_file(embedding, path).value();
            else {
                std::print("Error: {} labeled planar but is not\n", i);
                path = uncomputed / (std::to_string(i) + ".txt");
                loader::save_graph_to_file(graphs[i], path).value();
            }
            continue;
        } else if (std::holds_alternative<GraphType::Toroidal>(results[i])) {
            const auto& embedding = std::get<GraphType::Toroidal>(results[i]).embedding;
            path = toroidals / (std::to_string(i) + ".txt");
            if (!check_correctness || compute_embedding_genus(embedding) == 1)
                loader::save_embedding_to_file(embedding, path).value();
            else {
                std::print("Error: {} labeled toroidal but is not\n", i);
                path = uncomputed / (std::to_string(i) + ".txt");
                loader::save_graph_to_file(graphs[i], path).value();
            }
            continue;
        }
        if (std::holds_alternative<GraphType::NonBiconnected>(results[i])) {
            path = non_biconnected / (std::to_string(i) + ".txt");
            loader::save_graph_to_file(graphs[i], path).value();
            continue;
        }
        if (std::holds_alternative<GraphType::Uncomputed>(results[i])) {
            path = uncomputed / (std::to_string(i) + ".txt");
            std::print("Error: {} is uncomputed\n", i);
            loader::save_graph_to_file(graphs[i], path).value();
            continue;
        }
    }
}

} // namespace domus::torus::test