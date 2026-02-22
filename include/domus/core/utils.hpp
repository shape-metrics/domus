#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <algorithm>
#include <expected>
#include <functional>
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
    std::expected<void, std::string> insert(size_t index, T element) {
        if (has_element(element)) {
            std::string error_msg = "Error in CircularSequence::insert: ";
            error_msg += "Element already exists";
            return std::unexpected(error_msg);
        }
        auto it = m_elements.begin() + static_cast<typename std::vector<T>::difference_type>(index);
        m_elements.insert(it, element);
        recompute_positions();
        return {};
    }
    size_t next_index(const size_t index) const { return (index + 1) % size(); }
    void remove_if_exists(T element) {
        if (!has_element(element))
            return;
        const size_t position = *element_position(element);
        const auto erase_position =
            m_elements.begin() + static_cast<std::vector<int>::difference_type>(position);
        m_elements.erase(erase_position);
        recompute_positions();
    }
    T prev_element(T element) const {
        const size_t pos = *element_position(element);
        if (pos == 0)
            return at(size() - 1);
        return at(pos - 1);
    }
    T next_element(T element) const {
        const size_t pos = *element_position(element);
        if (pos == size() - 1)
            return at(0);
        return at(pos + 1);
    }
    bool has_element(T element) const { return m_element_position.contains(element); }
    std::expected<size_t, std::string> element_position(T element) const {
        if (!has_element(element)) {
            std::string error_msg = "Error in CircularSequence::element_position: ";
            error_msg += "Element not found";
            return std::unexpected(error_msg);
        }
        return m_element_position.at(element);
    }
    T operator[](const size_t index) const { return m_elements[index]; }
    T at(const size_t index) const { return m_elements.at(index); }
    [[nodiscard]] typename std::vector<T>::const_iterator begin() const {
        return m_elements.begin();
    }
    [[nodiscard]] typename std::vector<T>::const_iterator end() const { return m_elements.end(); }
};

class MemoryFile {
    char* buffer_m = nullptr;
    size_t size_m = 0;
    FILE* mem_m = nullptr;

    MemoryFile() = default;

  public:
    MemoryFile(const MemoryFile&) = delete;
    MemoryFile& operator=(const MemoryFile&) = delete;
    MemoryFile(MemoryFile&& other) noexcept
        : buffer_m(other.buffer_m), size_m(other.size_m), mem_m(other.mem_m) {
        other.buffer_m = nullptr;
        other.mem_m = nullptr;
        other.size_m = 0;
    }
    MemoryFile& operator=(MemoryFile&& other) noexcept {
        if (this != &other) {
            cleanup();
            buffer_m = other.buffer_m;
            size_m = other.size_m;
            mem_m = other.mem_m;
            other.buffer_m = nullptr;
            other.mem_m = nullptr;
            other.size_m = 0;
        }
        return *this;
    }
    static std::expected<MemoryFile, std::string> create() {
        MemoryFile mf;
        mf.mem_m = open_memstream(&mf.buffer_m, &mf.size_m);
        if (!mf.mem_m)
            return std::unexpected("MemoryFile: Failed to open_memstream");
        return mf;
    }
    ~MemoryFile() { cleanup(); }
    void cleanup() {
        if (mem_m) {
            fclose(mem_m);
            mem_m = nullptr;
        }
        if (buffer_m) {
            free(buffer_m);
            buffer_m = nullptr;
        }
    }
    FILE* get_file() { return mem_m; }
    const char* get_buffer() {
        if (mem_m)
            fflush(mem_m);
        return buffer_m;
    }
    size_t get_size() {
        if (mem_m)
            fflush(mem_m);
        return size_m;
    }
};

#endif