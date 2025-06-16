#include "sat/cnf_builder.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

#include "core/utils.hpp"

const std::string project_path = std::filesystem::current_path().string() + "/";
const std::string cnf_logs_file = "cnf_logs.txt";
std::mutex cnf_logs_mutex;

void CnfBuilder::add_clause(std::vector<int> clause) {
  for (int lit : clause) m_num_vars = std::max(m_num_vars, std::abs(lit));
  m_rows.push_back({CnfRowType::CLAUSE, std::move(clause), ""});
  m_num_clauses++;
}

void CnfBuilder::add_comment(const std::string& comment) {
  m_rows.push_back({CnfRowType::COMMENT, {}, comment});
}

int CnfBuilder::get_number_of_variables() const { return m_num_vars; }

int CnfBuilder::get_number_of_clauses() const { return m_num_clauses; }

void CnfBuilder::convert_to_cnf(const std::string& file_path) const {
  std::ofstream file(file_path);
  if (!file) {
    std::cerr << "Error: Could not open file " << file_path
              << " for writing.\n";
    return;
  }
  file << "p cnf " << get_number_of_variables() << " "
       << get_number_of_clauses() << "\n";
  for (const auto& row : m_rows) {
    switch (row.m_type) {
      case CnfRowType::COMMENT:
        file << "c " << row.m_comment << "\n";
        break;
      case CnfRowType::CLAUSE:
        for (int lit : row.m_clause) file << lit << " ";
        file << "0\n";
        break;
    }
  }
  file.close();
  std::lock_guard<std::mutex> lock(cnf_logs_mutex);
  std::ofstream log_file(cnf_logs_file, std::ios_base::app);
  if (log_file) {
    log_file << "v " << get_number_of_variables();
    log_file << " c " << get_number_of_clauses() << "\n";
    log_file.close();
  } else {
    throw std::runtime_error("Error: Could not open log file for writing: " +
                             cnf_logs_file);
  }
}