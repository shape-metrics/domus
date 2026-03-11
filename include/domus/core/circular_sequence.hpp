#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "domus/core/containers.hpp"

class CircularSequence {
    std::vector<size_t> m_elements{};
    Int_ToInt_HashMap m_index;
    void recompute_positions() {
        m_index.clear();
        for (size_t i = 0; i < m_elements.size(); i++)
            m_index.add(m_elements[i], i);
    }

  public:
    CircularSequence() {}
    explicit CircularSequence(std::ranges::input_range auto&& elements)
        : m_elements(std::begin(elements), std::end(elements)) {
        recompute_positions();
    }
    const std::vector<size_t>& get_elements() const { return m_elements; }
    void reverse() {
        std::ranges::reverse(m_elements);
        recompute_positions();
    }
    void clear() {
        m_elements.clear();
        m_index.clear();
    }
    bool empty() const { return m_elements.empty(); }
    size_t size() const { return m_elements.size(); }
    void append(size_t element) {
        m_elements.push_back(element);
        m_index.add(element, m_elements.size() - 1);
    }
    void insert(size_t index, size_t element) {
        assert(
            !has_element(element) && "Error in CircularSequence::insert: element already exists"
        );
        auto it =
            m_elements.begin() + static_cast<typename std::vector<size_t>::difference_type>(index);
        m_elements.insert(it, element);
        recompute_positions();
    }
    size_t next_index(const size_t index) const { return (index + 1) % size(); }
    void remove_if_exists(size_t element) {
        if (!has_element(element))
            return;
        size_t position = *element_position(element);
        auto erase_position =
            m_elements.begin() + static_cast<std::vector<size_t>::difference_type>(position);
        m_elements.erase(erase_position);
        recompute_positions();
    }
    size_t prev_element(size_t element) const {
        size_t pos = *element_position(element);
        if (pos == 0)
            return at(size() - 1);
        return at(pos - 1);
    }
    size_t next_element(size_t element) const {
        const size_t pos = *element_position(element);
        if (pos == size() - 1)
            return at(0);
        return at(pos + 1);
    }
    bool has_element(size_t element) const { return m_index.has(element); }
    std::optional<size_t> element_position(size_t element) const {
        assert(
            has_element(element) && "Error in CircularSequence::element_position: element not found"
        );
        return m_index.get(element);
    }
    size_t operator[](const size_t index) const { return m_elements[index]; }
    size_t at(const size_t index) const { return m_elements.at(index); }
    void for_each(std::function<void(size_t)> func) const {
        for (const auto& element : m_elements)
            func(element);
    }
};
