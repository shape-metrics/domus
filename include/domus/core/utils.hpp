#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

void save_string_to_file(const std::string& filename, const std::string& content);

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

std::string color_to_string(Color color);

Color string_to_color(const std::string& color);

std::string get_unique_filename(const std::string& base_filename, const std::string& folder);

std::string get_unique_filename(const std::string& base_filename);

struct int_pair_hash {
    int operator()(const std::pair<int, int>& p) const {
        const int h1 = static_cast<int>(std::hash<int>{}(p.first));
        const int h2 = static_cast<int>(std::hash<int>{}(p.second));
        const int mult = h2 * static_cast<int>(0x9e3779b9);
        return h1 ^ (mult + (h1 << 6) + (h1 >> 2));
    }
};

std::vector<std::string> collect_txt_files(const std::string& folder_path);

double compute_stddev(const std::vector<int>& values);

using IntPairHashSet = std::unordered_set<std::pair<int, int>, int_pair_hash>;

template <typename T> class CircularSequence {
    std::vector<T> m_elements;
    std::unordered_map<T, size_t> m_element_position;

  public:
    CircularSequence() = default;
    explicit CircularSequence(const std::vector<T>& elements) : m_elements(elements) {
        recompute_positions();
    }
    void reverse() {
        std::ranges::reverse(m_elements);
        recompute_positions();
    }
    void recompute_positions() {
        m_element_position.clear();
        for (size_t i = 0; i < size(); i++)
            m_element_position[at(i)] = i;
    }
    void clear() {
        m_elements.clear();
        m_element_position.clear();
    }
    [[nodiscard]] bool empty() const { return m_elements.empty(); }
    [[nodiscard]] size_t size() const { return m_elements.size(); }
    void append(T element) {
        m_elements.push_back(element);
        m_element_position[element] = size() - 1;
    }
    void insert(size_t index, T element) {
        if (has_element(element))
            throw std::runtime_error("Element already exists");
        auto it = m_elements.begin() + static_cast<typename std::vector<T>::difference_type>(index);
        m_elements.insert(it, element);
        recompute_positions();
    }
    [[nodiscard]] size_t next_index(const size_t index) const { return (index + 1) % size(); }
    void remove_if_exists(T element) {
        if (!has_element(element))
            return;
        const size_t position = element_position(element);
        const auto erase_position =
            m_elements.begin() + static_cast<std::vector<int>::difference_type>(position);
        m_elements.erase(erase_position);
        recompute_positions();
    }
    [[nodiscard]] T prev_element(T element) const {
        const size_t pos = element_position(element);
        if (pos == 0)
            return at(size() - 1);
        return at(pos - 1);
    }
    [[nodiscard]] T next_element(T element) const {
        const size_t pos = element_position(element);
        if (pos == size() - 1)
            return at(0);
        return at(pos + 1);
    }
    [[nodiscard]] bool has_element(T element) const { return m_element_position.contains(element); }
    [[nodiscard]] size_t element_position(T element) const {
        if (!has_element(element))
            throw std::runtime_error("Element not found");
        return m_element_position.at(element);
    }
    T operator[](const size_t index) const { return m_elements[index]; }
    [[nodiscard]] T at(const size_t index) const { return m_elements.at(index); }
    [[nodiscard]] typename std::vector<T>::const_iterator begin() const {
        return m_elements.begin();
    }
    [[nodiscard]] typename std::vector<T>::const_iterator end() const { return m_elements.end(); }
};

class MemoryFile {
    char* buffer = NULL;
    size_t size = 0;
    FILE* mem = NULL;

  public:
    MemoryFile() {
        mem = open_memstream(&buffer, &size);
        if (!mem) {
            perror("open_memstream");
            throw std::runtime_error("Failed to initialize MemoryFile");
        }
    }
    ~MemoryFile() {
        fclose(mem);
        free(buffer);
    }
    FILE* get_file() { return mem; }
    const char* get_buffer() const {
        fflush(mem);
        return buffer;
    }
};

#endif