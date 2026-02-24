#include "domus/core/utils.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <unordered_map>

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
    return std::unexpected(error_msg);
}

string color_to_string(const Color color) {
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
        assert(false && "Invalid color");
        return "Invalid color";
    }
}

Color string_to_color(const string& color) {
    if (color == "red")
        return Color::RED;
    if (color == "blue")
        return Color::BLUE;
    if (color == "green")
        return Color::GREEN;
    if (color == "black")
        return Color::BLACK;
    assert(false && "Invalid color string");
    return Color::BLACK;
}

expected<vector<string>, string> collect_txt_files(std::filesystem::path folder_path) {
    vector<string> txt_files;
    if (!filesystem::exists(folder_path)) {
        string error_msg = "Error in collect_txt_files: ";
        error_msg += "Folder does not exist: " + folder_path.string();
        return std::unexpected(error_msg);
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

class SequenceIndexImpl : public ISequenceIndex {
    std::unordered_map<size_t, size_t> m_map;

  public:
    void clear() override { m_map.clear(); }
    void insert(size_t hash, size_t pos) override { m_map[hash] = pos; }
    bool contains(size_t hash) const override { return m_map.contains(hash); }
    std::optional<size_t> get_position(size_t hash) const override {
        if (auto it = m_map.find(hash); it != m_map.end())
            return it->second;
        return std::nullopt;
    }
};

std::unique_ptr<ISequenceIndex> createSequenceIndex() {
    return std::make_unique<SequenceIndexImpl>();
}
