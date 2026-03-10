#include "domus/core/utils.hpp"

#include <cmath>
#include <fstream>
// #include <unistd.h>

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

double compute_stddev(const vector<int>& values) {
    if (values.size() <= 1)
        return 0.0;
    double mean = 0;
    for (const int value : values)
        mean += value;
    const auto size = static_cast<double>(values.size());
    mean /= size;
    double variance = 0.0;
    for (const int value : values)
        variance += (value - mean) * (value - mean);
    variance /= size - 1.0;
    return std::sqrt(variance);
}