#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <algorithm>
#include <cassert>
#include <expected>
#include <filesystem>
#include <memory>
#include <string>
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

class ISequenceIndex {
  public:
    virtual ~ISequenceIndex() = default;
    virtual void clear() = 0;
    virtual void insert(size_t hash, size_t pos) = 0;
    virtual bool contains(size_t hash) const = 0;
    virtual std::optional<size_t> get_position(size_t hash) const = 0;
};

std::unique_ptr<ISequenceIndex> createSequenceIndex();

template <typename T> class CircularSequence {
    std::vector<T> m_elements{};
    std::unique_ptr<ISequenceIndex> m_index = createSequenceIndex();
    void recompute_positions() {
        m_index->clear();
        for (size_t i = 0; i < m_elements.size(); i++)
            m_index->insert(get_hash(m_elements[i]), i);
    }
    size_t get_hash(const T& val) const { return std::hash<T>{}(val); }

  public:
    CircularSequence() {}
    explicit CircularSequence(std::ranges::input_range auto&& elements)
        : m_elements(std::begin(elements), std::end(elements)) {
        recompute_positions();
    }
    const std::vector<T>& get_elements() const { return m_elements; }
    void reverse() {
        std::ranges::reverse(m_elements);
        recompute_positions();
    }
    void clear() {
        m_elements.clear();
        m_index->clear();
    }
    bool empty() const { return m_elements.empty(); }
    size_t size() const { return m_elements.size(); }
    void append(T element) {
        m_elements.push_back(element);
        m_index->insert(get_hash(element), m_elements.size() - 1);
    }
    void insert(size_t index, T element) {
        assert(
            !has_element(element) && "Error in CircularSequence::insert: element already exists"
        );
        auto it = m_elements.begin() + static_cast<typename std::vector<T>::difference_type>(index);
        m_elements.insert(it, element);
        recompute_positions();
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
    bool has_element(const T& element) const { return m_index->contains(get_hash(element)); }
    std::optional<size_t> element_position(T element) const {
        assert(
            has_element(element) && "Error in CircularSequence::element_position: element not found"
        );
        return m_index->get_position(get_hash(element));
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