#include "domus/core/graph/graph_utilities.hpp"
#include "domus/core/containers.hpp"

#include <cassert>
#include <memory>

// NODE SET

void NodesContainer::add_node(int node_id) { add(node_id); }

bool NodesContainer::has_node(int node_id) const { return has(node_id); }

int NodesContainer::get_one_node_id() const {
    assert(!empty() && "NodesContainer::get_one_node_id: container is empty");
    return get_one_int();
}

size_t NodesContainer::size() const { return IntHashSet::size(); }

bool NodesContainer::empty() const { return IntHashSet::empty(); }

void NodesContainer::erase(int node_id) {
    assert(has_node(node_id) && "NodesContainer::erase: node does not exist");
    IntHashSet::erase(node_id);
}

void NodesContainer::for_each(std::function<void(int)> func) const { IntHashSet::for_each(func); }

// ADJACENCY LIST

class I_AdjacencyList {
  public:
    std::unordered_map<int, NodesContainer> map;
};

AdjacencyList::AdjacencyList() { m_impl = std::make_unique<I_AdjacencyList>(); }

AdjacencyList::~AdjacencyList() = default;

AdjacencyList::AdjacencyList(AdjacencyList&&) noexcept = default;

AdjacencyList& AdjacencyList::operator=(AdjacencyList&&) noexcept = default;

void AdjacencyList::add_edge(int from_id, int to_id) {
    assert(!has_edge(from_id, to_id) && "AdjacencyList::add_edge: edge already exists");
    m_impl->map[from_id].add_node(to_id);
}

bool AdjacencyList::has_edge(int from_id, int to_id) const {
    if (!m_impl->map.contains(from_id))
        return false;
    return m_impl->map.at(from_id).has_node(to_id);
}

const NodesContainer& AdjacencyList::get_neighbors_of_node(int node_id) const {
    return m_impl->map[node_id];
}

void AdjacencyList::erase_edge(int from_id, int to_id) {
    assert(has_edge(from_id, to_id) && "AdjacencyList::erase: edge does not exist");
    m_impl->map[from_id].erase(to_id);
}

void AdjacencyList::erase_node(int node_id) { m_impl->map.erase(node_id); }