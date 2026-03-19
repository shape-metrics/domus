#include "domus/core/circular_sequence.hpp"

#include <algorithm>

#include "domus_debug.hpp"

const std::vector<size_t>& CircularSequence::get_elements() const { return m_elements; }

void CircularSequence::reverse() { std::ranges::reverse(m_elements); }

void CircularSequence::clear() { m_elements.clear(); }

bool CircularSequence::empty() const { return m_elements.empty(); }

size_t CircularSequence::size() const { return m_elements.size(); }

void CircularSequence::append(size_t element) { m_elements.push_back(element); }

void CircularSequence::insert(size_t index, size_t element) {
    DOMUS_ASSERT(!has_element(element), "CircularSequence::insert: element already exists");
    auto it =
        m_elements.begin() + static_cast<typename std::vector<size_t>::difference_type>(index);
    m_elements.insert(it, element);
}

size_t CircularSequence::next_index(const size_t index) const { return (index + 1) % size(); }

void CircularSequence::remove_if_exists(size_t element) {
    if (!has_element(element))
        return;
    size_t position = *element_position(element);
    auto erase_position =
        m_elements.begin() + static_cast<std::vector<size_t>::difference_type>(position);
    m_elements.erase(erase_position);
}

bool CircularSequence::has_element(size_t element) const {
    return std::find(m_elements.begin(), m_elements.end(), element) != m_elements.end();
}

std::optional<size_t> CircularSequence::element_position(size_t element) const {
    DOMUS_ASSERT(has_element(element), "CircularSequence::element_position: element not found");
    auto it = std::find(m_elements.begin(), m_elements.end(), element);
    return std::distance(m_elements.begin(), it);
}

size_t CircularSequence::operator[](const size_t index) const { return m_elements[index % size()]; }

size_t CircularSequence::at(const size_t index) const { return m_elements.at(index % size()); }

void CircularSequence::for_each(std::function<void(size_t)> func) const {
    for (const auto& element : m_elements)
        func(element);
}
