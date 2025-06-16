#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

template <typename Iterable>
void print_iterable(const Iterable& container, const std::string& end = "\n") {
  std::cout << "[ ";
  for (const auto& elem : container) std::cout << elem << " ";
  std::cout << "]" << end;
}

template <typename T>
void print_array(T array[], int size, const std::string& end = "\n") {
  std::cout << "[ ";
  for (int i = 0; i < size; ++i) std::cout << array[i] << " ";
  std::cout << "]" << end;
}

void save_string_to_file(const std::string& filename,
                         const std::string& content);

enum class Color {
  RED,
  RED_SPECIAL,
  BLUE,
  BLUE_DARK,
  BLACK,
  GREEN,
  GREEN_DARK,
  RED_AND_BLUE,
  NONE,
  ANY,
};

const std::string color_to_string(const Color color);

std::string get_unique_filename(const std::string& base_filename,
                                const std::string& folder);

std::string get_unique_filename(const std::string& base_filename);

struct int_pair_hash {
  int operator()(const std::pair<int, int>& p) const {
    int h1 = std::hash<int>{}(p.first);
    int h2 = std::hash<int>{}(p.second);
    return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
  }
};

std::vector<std::string> collect_txt_files(const std::string& folder_path);

double compute_stddev(const std::vector<int>& values);

using IntPairHashSet = std::unordered_set<std::pair<int, int>, int_pair_hash>;

#endif