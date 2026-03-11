#include "domus/core/utils.hpp"

#include <cmath>
#include <fstream>

using namespace std;

expected<void, string> save_string_to_file(const string& filename, const string& content) {
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << content;
        outfile.close();
        return {};
    }
    string error_msg = "Error in save_string_to_file: ";
    error_msg += "Failed to open file for writing: " + filename;
    return unexpected(error_msg);
}

expected<vector<string>, string> collect_txt_files(std::filesystem::path folder_path) {
    vector<string> txt_files;
    if (!filesystem::exists(folder_path)) {
        string error_msg = "Error in collect_txt_files: ";
        error_msg += "Folder does not exist: " + folder_path.string();
        return unexpected(error_msg);
    }
    for (const auto& entry : filesystem::recursive_directory_iterator(folder_path))
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            txt_files.push_back(entry.path().string());
    return txt_files;
}

double compute_stddev(const vector<size_t>& values) {
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