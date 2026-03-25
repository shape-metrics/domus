#include "domus/core/utils.hpp"

#include <cmath>
#include <format>
#include <fstream>

namespace domus::utilities {

std::expected<void, std::string>
save_string_to_file(std::filesystem::path path, const std::string_view content) {
    std::ofstream outfile(path);
    if (outfile.is_open()) {
        outfile << content;
        outfile.close();
        return {};
    }
    return std::unexpected(
        std::format("save_string_to_file: failed to open file for writing: {}", path.c_str())
    );
}

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path) {
    std::vector<std::string> txt_files;
    if (!std::filesystem::exists(folder_path)) {
        return std::unexpected(
            std::format("collect_txt_files: folder does not exist: {}", folder_path.string())
        );
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path))
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            txt_files.push_back(entry.path().string());
    return txt_files;
}

double compute_stddev(const std::vector<size_t>& values) {
    if (values.size() <= 1)
        return 0.0;
    size_t sum = 0;
    for (size_t value : values)
        sum += value;
    const auto size = static_cast<double>(values.size());
    double mean = static_cast<double>(sum) / size;
    double variance = 0.0;
    for (size_t value : values) {
        double v = static_cast<double>(value);
        variance += (v - mean) * (v - mean);
    }
    variance /= (size - 1.0);
    return std::sqrt(variance);
}

} // namespace domus::utilities