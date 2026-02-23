#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <algorithm>
#include <expected>
#include <filesystem>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

std::expected<void, std::string>
save_string_to_file(const std::string& filename, const std::string& content);

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

struct int_pair_hash {
    int operator()(const std::pair<int, int>& p) const {
        const int h1 = static_cast<int>(std::hash<int>{}(p.first));
        const int h2 = static_cast<int>(std::hash<int>{}(p.second));
        const int mult = h2 * static_cast<int>(0x9e3779b9);
        return h1 ^ (mult + (h1 << 6) + (h1 >> 2));
    }
};

std::expected<std::vector<std::string>, std::string>
collect_txt_files(std::filesystem::path folder_path);

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
    bool empty() const { return m_elements.empty(); }
    size_t size() const { return m_elements.size(); }
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
    typename std::vector<T>::const_iterator begin() const { return m_elements.begin(); }
    typename std::vector<T>::const_iterator end() const { return m_elements.end(); }
};

class MemoryFile {
    struct State {
        char* buffer = nullptr;
        size_t size = 0;
        FILE* mem = nullptr;

        ~State() {
            if (mem)
                fclose(mem);
            if (buffer)
                free(buffer);
        }
    };
    std::unique_ptr<State> state;
    MemoryFile() : state(std::make_unique<State>()) {}

  public:
    MemoryFile(MemoryFile&&) noexcept = default;
    MemoryFile& operator=(MemoryFile&&) noexcept = default;

    MemoryFile(const MemoryFile&) = delete;
    MemoryFile& operator=(const MemoryFile&) = delete;

    static std::expected<MemoryFile, std::string> create() {
        MemoryFile mf;
        mf.state->mem = open_memstream(&mf.state->buffer, &mf.state->size);

        if (!mf.state->mem)
            return std::unexpected("MemoryFile: Failed to open_memstream");
        return mf;
    }

    FILE* get_file() { return state->mem; }

    const char* get_buffer() {
        if (state->mem)
            fflush(state->mem);
        return state->buffer;
    }

    size_t get_size() {
        if (state->mem)
            fflush(state->mem);
        return state->size;
    }
};

#endif