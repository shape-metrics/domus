#include "domus/core/circular_sequence.hpp"

#include <algorithm>

#include "domus_assert.hpp"

void CircularSequence::recompute_positions() {
    m_index.clear();
    for (size_t i = 0; i < m_elements.size(); i++)
        m_index.add(m_elements[i], i);
}

const std::vector<size_t>& CircularSequence::get_elements() const { return m_elements; }

void CircularSequence::reverse() {
    std::ranges::reverse(m_elements);
    recompute_positions();
}

void CircularSequence::clear() {
    m_elements.clear();
    m_index.clear();
}

bool CircularSequence::empty() const { return m_elements.empty(); }

size_t CircularSequence::size() const { return m_elements.size(); }

void CircularSequence::append(size_t element) {
    m_elements.push_back(element);
    m_index.add(element, m_elements.size() - 1);
}

void CircularSequence::insert(size_t index, size_t element) {
    DOMUS_ASSERT(!has_element(element), "CircularSequence::insert: element already exists");
    auto it =
        m_elements.begin() + static_cast<typename std::vector<size_t>::difference_type>(index);
    m_elements.insert(it, element);
    recompute_positions();
}

size_t CircularSequence::next_index(const size_t index) const { return (index + 1) % size(); }

void CircularSequence::remove_if_exists(size_t element) {
    if (!has_element(element))
        return;
    size_t position = *element_position(element);
    auto erase_position =
        m_elements.begin() + static_cast<std::vector<size_t>::difference_type>(position);
    m_elements.erase(erase_position);
    recompute_positions();
}

size_t CircularSequence::prev_element(size_t element) const {
    size_t pos = *element_position(element);
    if (pos == 0)
        return at(size() - 1);
    return at(pos - 1);
}

size_t CircularSequence::next_element(size_t element) const {
    const size_t pos = *element_position(element);
    if (pos == size() - 1)
        return at(0);
    return at(pos + 1);
}

bool CircularSequence::has_element(size_t element) const { return m_index.has(element); }

std::optional<size_t> CircularSequence::element_position(size_t element) const {
    DOMUS_ASSERT(has_element(element), "CircularSequence::element_position: element not found");
    return m_index.get(element);
}

size_t CircularSequence::operator[](const size_t index) const { return m_elements[index]; }

size_t CircularSequence::at(const size_t index) const { return m_elements.at(index); }

void CircularSequence::for_each(std::function<void(size_t)> func) const {
    for (const auto& element : m_elements)
        func(element);
}
