#include "domus/sat/cnf.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace std;

const string project_path = std::filesystem::current_path().string() + "/";
const string cnf_logs_file = "cnf_logs.txt";
std::mutex cnf_logs_mutex;

void Cnf::add_clause(vector<int> clause) {
    for (const int lit : clause)
        m_num_vars = max(m_num_vars, abs(lit));
    m_rows.push_back({CnfRowType::CLAUSE, std::move(clause), ""});
    m_num_clauses++;
}

void Cnf::add_comment(const string& comment) {
    m_rows.push_back({CnfRowType::COMMENT, {}, comment});
}

int Cnf::get_number_of_variables() const { return m_num_vars; }

int Cnf::get_number_of_clauses() const { return m_num_clauses; }

void Cnf::save_to_file(const string& file_path) const {
    std::ofstream file(file_path);
    if (!file) {
        std::cerr << "Error: Could not open file " << file_path << " for writing.\n";
        return;
    }
    file << to_string();
    std::lock_guard lock(cnf_logs_mutex);
    std::ofstream log_file(cnf_logs_file, std::ios_base::app);
    if (log_file) {
        log_file << "v " << get_number_of_variables();
        log_file << " c " << get_number_of_clauses() << "\n";
        log_file.close();
    } else {
        throw runtime_error("Error: Could not open log file for writing: " + cnf_logs_file);
    }
}

const string Cnf::to_string() const {
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

void Cnf::print() const { std::cout << to_string() << std::endl; }

const vector<CnfRow>& Cnf::get_rows() const { return m_rows; }