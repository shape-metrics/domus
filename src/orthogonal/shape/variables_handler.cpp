#include "orthogonal/shape/variables_handler.hpp"

void VariablesHandler::add_variable(int i, int j, Direction direction) {
  variable_to_edge[m_next_var] = std::make_pair(i, j);
  variable_to_direction[m_next_var] = direction;
  switch (direction) {
    case Direction::UP:
      m_edge_up_variable[{i, j}] = m_next_var;
      m_edge_down_variable[{j, i}] = m_next_var;
      break;
    case Direction::DOWN:
      m_edge_down_variable[{i, j}] = m_next_var;
      m_edge_up_variable[{j, i}] = m_next_var;
      break;
    case Direction::LEFT:
      m_edge_left_variable[{i, j}] = m_next_var;
      m_edge_right_variable[{j, i}] = m_next_var;
      break;
    case Direction::RIGHT:
      m_edge_right_variable[{i, j}] = m_next_var;
      m_edge_left_variable[{j, i}] = m_next_var;
      break;
  }
  m_next_var++;
}

void VariablesHandler::add_edge_variables(int i, int j) {
  add_variable(i, j, Direction::UP);
  add_variable(i, j, Direction::DOWN);
  add_variable(i, j, Direction::LEFT);
  add_variable(i, j, Direction::RIGHT);
}

VariablesHandler::VariablesHandler(const Graph& graph) {
  for (const auto& node : graph.get_nodes()) {
    int i = node.get_id();
    for (auto& edge : node.get_edges()) {
      int j = edge.get_to().get_id();
      if (i > j) continue;
      add_edge_variables(i, j);
    }
  }
}

int VariablesHandler::get_up_variable(int i, int j) const {
  return m_edge_up_variable.at({i, j});
}

int VariablesHandler::get_down_variable(int i, int j) const {
  return m_edge_down_variable.at({i, j});
}

int VariablesHandler::get_left_variable(int i, int j) const {
  return m_edge_left_variable.at({i, j});
}

int VariablesHandler::get_right_variable(int i, int j) const {
  return m_edge_right_variable.at({i, j});
}

int VariablesHandler::get_variable(int i, int j, Direction direction) const {
  if (direction == Direction::UP) return get_up_variable(i, j);
  if (direction == Direction::DOWN) return get_down_variable(i, j);
  if (direction == Direction::LEFT) return get_left_variable(i, j);
  if (direction == Direction::RIGHT) return get_right_variable(i, j);
  throw std::invalid_argument("Invalid direction");
}

const std::pair<int, int>& VariablesHandler::get_edge_of_variable(
    int variable) const {
  return variable_to_edge.at(variable);
}

Direction VariablesHandler::get_direction_of_edge(int i, int j) const {
  if (get_variable_value(get_up_variable(i, j))) return Direction::UP;
  if (get_variable_value(get_down_variable(i, j))) return Direction::DOWN;
  if (get_variable_value(get_left_variable(i, j))) return Direction::LEFT;
  if (get_variable_value(get_right_variable(i, j))) return Direction::RIGHT;
  throw std::runtime_error("No direction found for standard edge");
}

void VariablesHandler::set_variable_value(int variable, bool value) {
  if (variable_to_value.contains(variable))
    throw std::runtime_error("variable value is already set");
  variable_to_value[variable] = value;
}

bool VariablesHandler::get_variable_value(int variable) const {
  if (!variable_to_value.contains(variable))
    throw std::runtime_error("variable does not have a set value");
  return variable_to_value.at(variable);
}