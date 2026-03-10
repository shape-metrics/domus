#include "domus/core/containers.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <unordered_set>

struct int_pair_hash {
    int operator()(const std::pair<int, int>& p) const {
        const int h1 = static_cast<int>(std::hash<int>{}(p.first));
        const int h2 = static_cast<int>(std::hash<int>{}(p.second));
        const int mult = h2 * static_cast<int>(0x9e3779b9);
        return h1 ^ (mult + (h1 << 6) + (h1 >> 2));
    }
};

// INT PAIR SET

class I_IntPairHashSet {
  public:
    std::unordered_set<std::pair<int, int>, int_pair_hash> pairs;
};

PairIntHashSet::PairIntHashSet() { m_impl = std::make_unique<I_IntPairHashSet>(); }

PairIntHashSet::~PairIntHashSet() = default;

PairIntHashSet::PairIntHashSet(PairIntHashSet&&) noexcept = default;

PairIntHashSet& PairIntHashSet::operator=(PairIntHashSet&&) noexcept = default;

void PairIntHashSet::add(int value_1, int value_2) {
    assert(!has(value_1, value_2) && "PairIntHashSet::add: pair already exists");
    m_impl->pairs.insert({value_1, value_2});
}

bool PairIntHashSet::has(int value_1, int value_2) const {
    return m_impl->pairs.contains({value_1, value_2});
}

size_t PairIntHashSet::size() const { return m_impl->pairs.size(); }

bool PairIntHashSet::empty() const { return m_impl->pairs.empty(); }

void PairIntHashSet::erase(int value_1, int value_2) {
    assert(has(value_1, value_2) && "PairIntHashSet::erase: pair does not exist");
    m_impl->pairs.erase({value_1, value_2});
}

void PairIntHashSet::for_each(std::function<void(int, int)> func) const {
    for (const auto& [value_1, value_2] : m_impl->pairs)
        func(value_1, value_2);
}

void PairIntHashSet::clear() { m_impl->pairs.clear(); }

// INTPAIR TO INT MAP

class I_IntPair_ToInt_HashMap {
  public:
    std::unordered_map<std::pair<int, int>, int, int_pair_hash> map;
};

IntPair_ToInt_HashMap::IntPair_ToInt_HashMap() {
    m_impl = std::make_unique<I_IntPair_ToInt_HashMap>();
}

IntPair_ToInt_HashMap::~IntPair_ToInt_HashMap() = default;

IntPair_ToInt_HashMap::IntPair_ToInt_HashMap(IntPair_ToInt_HashMap&&) noexcept = default;

IntPair_ToInt_HashMap& IntPair_ToInt_HashMap::operator=(IntPair_ToInt_HashMap&&) noexcept = default;

void IntPair_ToInt_HashMap::add(int key_1, int key_2, int value) {
    assert(!has(key_1, key_2) && "IntPair_ToInt_HashMap::add: pair already exists");
    m_impl->map[{key_1, key_2}] = value;
}

int IntPair_ToInt_HashMap::get(int key_1, int key_2) const {
    return m_impl->map.at({key_1, key_2});
}

bool IntPair_ToInt_HashMap::has(int key_1, int key_2) const {
    return m_impl->map.contains({key_1, key_2});
}

size_t IntPair_ToInt_HashMap::size() const { return m_impl->map.size(); }

bool IntPair_ToInt_HashMap::empty() const { return m_impl->map.empty(); }

void IntPair_ToInt_HashMap::erase(int key_1, int key_2) {
    assert(has(key_1, key_2) && "IntPair_ToInt_HashMap::erase: pair does not exist");
    m_impl->map.erase({key_1, key_2});
}

// INT SET

class I_IntHashSet {
  public:
    std::unordered_set<int> ints;
};

IntHashSet::IntHashSet() { m_impl = std::make_unique<I_IntHashSet>(); }

IntHashSet::~IntHashSet() = default;

IntHashSet::IntHashSet(IntHashSet&&) noexcept = default;

IntHashSet& IntHashSet::operator=(IntHashSet&&) noexcept = default;

void IntHashSet::add(int value) { m_impl->ints.insert(value); }

bool IntHashSet::has(int value) const { return m_impl->ints.contains(value); }

int IntHashSet::get_one_int() const {
    assert(!empty() && "IntContainer::get_one_int: container is empty");
    return *m_impl->ints.begin();
}

size_t IntHashSet::size() const { return m_impl->ints.size(); }

bool IntHashSet::empty() const { return m_impl->ints.empty(); }

void IntHashSet::erase(int value) {
    assert(has(value) && "IntContainer::erase: value does not exist");
    m_impl->ints.erase(value);
}

void IntHashSet::for_each(std::function<void(int)> func) const {
    for (int id : m_impl->ints)
        func(id);
}

// INT TO INTSET HASHMAP

class I_Int_ToIntContainer_HashMap {
  public:
    std::unordered_map<int, IntHashSet> key_values;
};

Int_ToIntContainer_HashMap::Int_ToIntContainer_HashMap() {
    m_impl = std::make_unique<I_Int_ToIntContainer_HashMap>();
}

Int_ToIntContainer_HashMap::~Int_ToIntContainer_HashMap() = default;

Int_ToIntContainer_HashMap::Int_ToIntContainer_HashMap(Int_ToIntContainer_HashMap&&) noexcept =
    default;

Int_ToIntContainer_HashMap&
Int_ToIntContainer_HashMap::operator=(Int_ToIntContainer_HashMap&&) noexcept = default;

void Int_ToIntContainer_HashMap::add(int key, int value) {
    assert(!has(key, value) && "Int_ToIntContainer_HashMap::add: key-value already exists");
    m_impl->key_values[key].add(value);
}

bool Int_ToIntContainer_HashMap::has(int key, int value) const {
    if (!m_impl->key_values.contains(key))
        return false;
    return m_impl->key_values.at(key).has(value);
}

const IntHashSet& Int_ToIntContainer_HashMap::get(int node_id) const {
    return m_impl->key_values[node_id];
}

void Int_ToIntContainer_HashMap::erase(int key, int value) {
    assert(has(key, value) && "Int_ToIntContainer_HashMap::erase: key-value does not exist");
    m_impl->key_values[key].erase(value);
}

void Int_ToIntContainer_HashMap::erase(int node_id) { m_impl->key_values.erase(node_id); }

class I_Int_ToInt_HashMap {
  public:
    std::unordered_map<int, int> map;
};

Int_ToInt_HashMap::Int_ToInt_HashMap() { m_impl = std::make_unique<I_Int_ToInt_HashMap>(); }

Int_ToInt_HashMap::~Int_ToInt_HashMap() = default;

Int_ToInt_HashMap::Int_ToInt_HashMap(Int_ToInt_HashMap&&) noexcept = default;

Int_ToInt_HashMap& Int_ToInt_HashMap::operator=(Int_ToInt_HashMap&&) noexcept = default;

void Int_ToInt_HashMap::add(int key, int value) { m_impl->map[key] = value; }

bool Int_ToInt_HashMap::has(int key) const { return m_impl->map.contains(key); }

int Int_ToInt_HashMap::get(int key) const { return m_impl->map.at(key); }

void Int_ToInt_HashMap::erase(int key) { m_impl->map.erase(key); }

void Int_ToInt_HashMap::for_each(std::function<void(int, int)> func) const {
    for (const auto& [value_1, value_2] : m_impl->map)
        func(value_1, value_2);
}

void Int_ToInt_HashMap::clear() { m_impl->map.clear(); }

void Int_ToInt_HashMap::update(int key, int value) {
    assert(has(key));
    m_impl->map.at(key) = value;
}

int& Int_ToInt_HashMap::operator[](int key) { return m_impl->map[key]; }