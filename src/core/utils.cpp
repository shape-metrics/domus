#include "domus/core/utils.hpp"

#include <unistd.h> // For close

#include <cmath>
#include <filesystem>
#include <fstream> // To read the file later

#ifdef __linux__
constexpr char TEMPORARY_FOLDER[] = "/dev/shm/";
#elif __APPLE__
const char TEMPORARY_FOLDER[] = "/tmp/";
#endif

std::string get_unique_filename(const std::string& base_filename, const std::string& folder) {
    // Create a unique temporary file
    std::string filename_template = folder + base_filename + "_XXXXXX";
    const int fd = mkstemp(filename_template.data());
    if (fd == -1)
        throw std::runtime_error("Failed to create unique temporary file");
    close(fd); // Close the file descriptor
    return filename_template;
}

#if defined(__linux__) || defined(__APPLE__)
std::string get_unique_filename(const std::string& base_filename) {
    return get_unique_filename(base_filename, TEMPORARY_FOLDER);
}
#endif

void save_string_to_file(const std::string& filename, const std::string& content) {
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << content;
        outfile.close();
    } else
        throw std::runtime_error("Failed to open file for writing: " + filename);
}

std::string color_to_string(const Color color) {
    switch (color) {
    case Color::RED:
        return "red";
    case Color::BLUE:
        return "blue";
    case Color::BLACK:
        return "black";
    case Color::GREEN:
        return "green";
    case Color::RED_SPECIAL:
        return "darkred";
    default:
        throw std::invalid_argument("Invalid color");
    }
}

Color string_to_color(const std::string& color) {
    if (color == "red")
        return Color::RED;
    if (color == "blue")
        return Color::BLUE;
    if (color == "green")
        return Color::GREEN;
    if (color == "black")
        return Color::BLACK;
    throw std::invalid_argument("Invalid color string");
}

std::vector<std::string> collect_txt_files(const std::string& folder_path) {
    std::vector<std::string> txt_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path))
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            txt_files.push_back(entry.path().string());
    return txt_files;
}

double compute_stddev(const std::vector<int>& values) {
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