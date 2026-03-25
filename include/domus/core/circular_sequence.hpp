#pragma once

#include <functional>
#include <optional>
#include <vector>

// TODO remove?
class CircularSequence {
    std::vector<size_t> m_elements{};

  public:
    CircularSequence() {}
    explicit CircularSequence(std::ranges::input_range auto&& elements)
        : m_elements(std::begin(elements), std::end(elements)) {}
    const std::vector<size_t>& get_elements() const;
    void reverse();
    void clear();
    bool empty() const;
    size_t size() const;
    void append(size_t element);
    void insert(size_t index, size_t element);
    size_t next_index(const size_t index) const;
    void remove_if_exists(size_t element);
    bool has_element(size_t element) const;
    std::optional<size_t>
    element_position(size_t element) const; // TODO should use assert and return size_t
    size_t operator[](const size_t index) const;
    size_t at(const size_t index) const;
    void for_each(std::function<void(size_t)> func) const;
};
