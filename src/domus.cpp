#include <filesystem>
#include <print>
#include <string_view>
#include <vector>

#include "domus/core/graph/file_loader.hpp"
#include "domus/torus/test.hpp"

using namespace domus;

namespace {

void print_usage(std::string_view prog_name) {
    std::println(stderr, "Usage: {} <dataset> <obstructions_directory> [OPTIONS]", prog_name);
    std::println(stderr, "\nPositional arguments:");
    std::println(stderr, "  <dataset>                 Path to dataset file (.asc)");
    std::println(stderr, "  <obstructions_directory>  Output directory for obstructions");
    std::println(stderr, "  [check_correctness]       Optional boolean (true/false, 1/0)");
    std::println(stderr, "\nOptions:");
    std::println(stderr, "  -c, --check-correctness   Enable correctness check");
    std::println(stderr, "  --no-check-correctness    Disable correctness check (default)");
    std::println(stderr, "  -h, --help                Show this help message");
}

bool parse_bool(std::string_view val, bool default_val = false) {
    if (val == "1" || val == "true" || val == "TRUE" || val == "yes" || val == "YES" ||
        val == "on" || val == "ON" || val == "t" || val == "y") {
        return true;
    }
    if (val == "0" || val == "false" || val == "FALSE" || val == "no" || val == "NO" ||
        val == "off" || val == "OFF" || val == "f" || val == "n") {
        return false;
    }
    std::println(
        stderr,
        "Warning: Unrecognized boolean value '{}', defaulting to {}.",
        val,
        default_val
    );
    return default_val;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::vector<std::string_view> positional_args;
    bool check_correctness = false;
    bool flag_specified = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-c" || arg == "--check-correctness" || arg == "--check") {
            check_correctness = true;
            flag_specified = true;
        } else if (arg == "--no-check-correctness" || arg == "--no-check") {
            check_correctness = false;
            flag_specified = true;
        } else if (arg.starts_with("--check-correctness=")) {
            check_correctness = parse_bool(arg.substr(20));
            flag_specified = true;
        } else if (arg.starts_with("--check=")) {
            check_correctness = parse_bool(arg.substr(8));
            flag_specified = true;
        } else {
            positional_args.push_back(arg);
        }
    }

    if (positional_args.size() < 2) {
        std::println(stderr, "Error: Missing required positional arguments.");
        print_usage(argv[0]);
        return 1;
    }

    std::filesystem::path dataset = positional_args[0];
    std::filesystem::path obstructions_directory = positional_args[1];

    if (!flag_specified && positional_args.size() >= 3) {
        check_correctness = parse_bool(positional_args[2]);
    }

    auto graphs = graph::loader::load_graphs_from_asc_file(dataset);
    if (!graphs.has_value()) {
        std::println(stderr, "Error: Failed to load dataset from {}", dataset.string());
        return 1;
    }

    const auto result = torus::test::find_minimal_obstructions(*graphs);

    torus::test::save_all_results(*graphs, result, obstructions_directory);

    if (check_correctness)
        torus::test::check_all_results(*graphs, result);

    return 0;
}
