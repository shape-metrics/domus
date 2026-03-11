#include "domus/sat/cnf.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <print>
#include <sstream>
#include <utility>

const std::string project_path = std::filesystem::current_path().string() + "/";
const std::string cnf_logs_file = "cnf_logs.txt";
std::mutex cnf_logs_mutex;

void Cnf::add_clause(std::vector<int> clause) {
    for (int lit : clause)
        m_num_vars = static_cast<size_t>(std::max(static_cast<int>(m_num_vars), abs(lit)));
    m_rows.push_back({CnfRowType::CLAUSE, std::move(clause), ""});
    m_num_clauses++;
}

void Cnf::add_comment(const std::string& comment) {
    m_rows.push_back({CnfRowType::COMMENT, {}, comment});
}

size_t Cnf::get_number_of_variables() const { return m_num_vars; }

size_t Cnf::get_number_of_clauses() const { return m_num_clauses; }

std::expected<void, std::string> Cnf::save_to_file(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        std::string error_msg = "Cnf::save_to_file: could not open file ";
        error_msg += file_path;
        return std::unexpected(error_msg);
    }
    file << to_string();
    std::lock_guard lock(cnf_logs_mutex);
    std::ofstream log_file(cnf_logs_file, std::ios_base::app);
    if (!log_file) {
        std::string error_msg = "Cnf::save_to_file: could not open file ";
        error_msg += cnf_logs_file;
        return std::unexpected(error_msg);
    }
    log_file << "v " << get_number_of_variables();
    log_file << " c " << get_number_of_clauses() << "\n";
    log_file.close();
    return {};
}

std::string Cnf::to_string() const {
    std::stringstream ss;
    ss << "p cnf " << get_number_of_variables() << " " << get_number_of_clauses() << "\n";
    for (const auto& row : m_rows) {
        switch (row.m_type) {
        case CnfRowType::COMMENT:
            ss << "c " << row.m_comment << "\n";
            break;
        case CnfRowType::CLAUSE:
            for (int lit : row.m_clause)
                ss << lit << " ";
            ss << "0\n";
            break;
        }
    }
    return ss.str();
}

void Cnf::print() const { println("{}", to_string()); }

const std::vector<CnfRow>& Cnf::get_rows() const { return m_rows; }