#pragma once

#include <bitset>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace domus::utilities {

std::expected<void, std::string>
save_string_to_file(std::filesystem::path path, const std::string_view content);

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path);

double compute_stddev(const std::vector<size_t>& values);

template <size_t N> std::vector<std::bitset<N>> generate_all_bitsets() {
    // There are 2^N possible combinations
    size_t num_combinations = 1ULL << N;
    std::vector<std::bitset<N>> result;
    result.reserve(num_combinations);
    for (size_t i = 0; i < num_combinations; ++i)
        result.push_back(std::bitset<N>(i));
    return result;
}

} // namespace domus::utilities