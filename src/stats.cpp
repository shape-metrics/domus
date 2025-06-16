#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "config/config.hpp"
#include "core/csv.hpp"
#include "core/graph/file_loader.hpp"
#include "core/graph/graph.hpp"
#include "core/graph/graphs_algorithms.hpp"
#include "orthogonal/drawing_builder.hpp"
#include "orthogonal/drawing_stats.hpp"
#include "orthogonal/file_loader.hpp"

std::unordered_set<std::string> graphs_already_in_csv;
int total_fails = 0;
std::mutex input_output_lock;
std::mutex write_lock;

auto test_shape_metrics_approach(
    const Graph& graph, const std::filesystem::path& svg_output_filename) {
  auto start = std::chrono::high_resolution_clock::now();
  DrawingResult result = make_orthogonal_drawing(graph);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  make_svg(*result.augmented_graph, result.attributes, svg_output_filename);
  return std::make_pair(std::move(result), elapsed.count());
}

void save_stats(std::ofstream& results_file, const DrawingResult& result,
                double time, const std::string& graph_name) {
  std::lock_guard<std::mutex> lock(write_lock);
  auto stats = compute_all_orthogonal_stats(result);
  results_file << graph_name << ",";
  results_file << stats.crossings << ",";
  results_file << stats.bends << ",";
  results_file << stats.area << ",";
  results_file << stats.total_edge_length << ",";
  results_file << stats.max_edge_length << ",";
  results_file << stats.max_bends_per_edge << ",";
  results_file << stats.edge_length_stddev << ",";
  results_file << stats.bends_stddev << ",";
  results_file << time << ",";
  results_file << result.initial_number_of_cycles << ",";
  results_file << result.number_of_added_cycles << ",";
  results_file << result.number_of_useless_bends;
  results_file << std::endl;
}

void make_stats_of_graphs_in_folder(const std::string& folder_path,
                                    std::ofstream& results_file,
                                    const std::string& output_svgs_folder) {
  auto txt_files = collect_txt_files(folder_path);
  std::atomic<int> number_of_drawings_computed{0};
  std::atomic<int> index{0};
  unsigned num_threads = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  for (unsigned i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      while (true) {
        int current = index.fetch_add(1, std::memory_order_relaxed);
        if (current >= txt_files.size()) break;
        const auto& entry_path = txt_files[current];
        const std::string graph_filename =
            std::filesystem::path(entry_path).stem().string();
        int current_number =
            number_of_drawings_computed.fetch_add(1, std::memory_order_relaxed);
        if (graphs_already_in_csv.contains(graph_filename)) continue;
        std::unique_ptr<Graph> graph = load_graph_from_txt_file(entry_path);
        if (!is_graph_connected(*graph)) {
          std::lock_guard<std::mutex> lock(input_output_lock);
          std::cerr << "Graph " << graph_filename
                    << " is not connected, skipping." << std::endl;
          continue;
        }
        {
          std::lock_guard<std::mutex> lock(input_output_lock);
          std::cout << "Processing comparison #" << current_number << " - "
                    << graph_filename << std::endl;
        }

        std::filesystem::path svg_output_path =
            std::filesystem::path(output_svgs_folder) /
            (graph_filename + ".svg");
        try {
          auto result = test_shape_metrics_approach(*graph, svg_output_path);
          save_stats(results_file, result.first, result.second, graph_filename);
        } catch (const std::exception& e) {
          std::lock_guard<std::mutex> lock(input_output_lock);
          std::cout << "Error processing graph " << graph_filename << std::endl;
          if (std::string(e.what()) != "Graph contains cycle") {
            std::cerr << "Error: " << e.what() << std::endl;
            throw;
          } else {
            std::cout << "Graph contains cycle, skipping." << std::endl;
            total_fails++;
            throw;
          }
        }
      }
    });
  }
  for (auto& t : threads)
    if (t.joinable()) t.join();
  std::cout << "All stats computed." << std::endl;
  std::cout << "Threads used: " << num_threads << std::endl;
  std::cout << "Total fails: " << total_fails << std::endl;
  std::cout << "Total graphs: " << number_of_drawings_computed.load()
            << std::endl;
}

void initialize_csv_file(std::ofstream& result_file) {
  if (!result_file.is_open())
    throw std::runtime_error("Error: Could not open result file");
  result_file
      << "graph_name,crossings,bends,area,total_edge_length,max_edge_length,"
      << "max_bends_per_edge,edge_length_stddev,bends_stddev,time,"
      << "initial_number_cycles,number_added_cycles,number_useless_bends"
      << std::endl;
}

void make_stats(const Config& config) {
  std::string test_results_filename = config.get("output_result_filename");
  std::ofstream result_file;
  if (std::filesystem::exists(test_results_filename)) {
    std::cout << "File " << test_results_filename << " already exists."
              << std::endl;
    std::cout << "What do you want to do?" << std::endl;
    std::cout << "1. Overwrite the file" << std::endl;
    std::cout << "2. Append to the file" << std::endl;
    std::cout << "3. Abort" << std::endl;
    std::cout << "Please enter your choice (1/2/3): ";
    int choice;
    std::cin >> choice;
    if (choice == 1) {
      std::filesystem::remove(test_results_filename);
      result_file.open(test_results_filename);
      initialize_csv_file(result_file);
    } else if (choice == 2) {
      auto csv_data = parse_csv(test_results_filename);
      for (const auto& row : csv_data.rows)
        if (row.size() > 0) graphs_already_in_csv.insert(row[0]);
      result_file.open(test_results_filename, std::ios_base::app);
    } else {
      std::cout << "Aborting." << std::endl;
      return;
    }
  } else {
    result_file.open(test_results_filename);
    initialize_csv_file(result_file);
  }
  std::string output_svgs_folder = config.get("output_svgs_folder");
  if (!std::filesystem::exists(output_svgs_folder))
    if (!std::filesystem::create_directories(output_svgs_folder)) {
      std::cerr << "Error: Could not create directory " << output_svgs_folder
                << std::endl;
      return;
    }
  std::string test_graphs_folder = config.get("test_graphs_folder");
  make_stats_of_graphs_in_folder(test_graphs_folder, result_file,
                                 output_svgs_folder);
  std::cout << std::endl;
  result_file.close();
}

int main() {
  Config config("config.txt");
  make_stats(config);
  return 0;
}
