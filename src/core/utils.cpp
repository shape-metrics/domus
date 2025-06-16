#include "core/utils.hpp"

#include <math.h>
#include <unistd.h>  // For close

#include <cstdlib>  // For mkstemp, system, remove
#include <filesystem>
#include <fstream>  // To read the file later
#include <sstream>  // For std::stringstream

#ifdef __linux__
const char TEMPORARY_FOLDER[] = "/dev/shm/";
#elif __APPLE__
const char TEMPORARY_FOLDER[] = "/tmp/";
#endif

std::string get_unique_filename(const std::string& base_filename,
                                const std::string& folder) {
  // Create a unique temporary file
  std::string filename_template = folder + base_filename + "_XXXXXX";
  int fd = mkstemp(filename_template.data());
  if (fd == -1)
    throw std::runtime_error("Failed to create unique temporary file");
  close(fd);  // Close the file descriptor
  return filename_template;
}

std::string get_unique_filename(const std::string& base_filename) {
  return get_unique_filename(base_filename, TEMPORARY_FOLDER);
}

void save_string_to_file(const std::string& filename,
                         const std::string& content) {
  std::ofstream outfile(filename);
  if (outfile.is_open()) {
    outfile << content;
    outfile.close();
  } else
    throw std::runtime_error("Failed to open file for writing: " + filename);
}

const std::string color_to_string(const Color color) {
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

std::vector<std::string> collect_txt_files(const std::string& folder_path) {
  std::vector<std::string> txt_files;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(folder_path))
    if (entry.is_regular_file() && entry.path().extension() == ".txt")
      txt_files.push_back(entry.path().string());
  return txt_files;
}

double compute_stddev(const std::vector<int>& values) {
  if (values.size() <= 1) return 0.0;
  double mean = 0;
  for (const auto& value : values) mean += value;
  mean /= values.size();
  double variance = 0;
  for (const auto& value : values) variance += (value - mean) * (value - mean);
  variance /= (values.size() - 1);
  return std::sqrt(variance);
}