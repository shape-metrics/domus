#include "domus/core/csv.hpp"

#include <cstddef>
#include <fstream>
#include <sstream>

using namespace std;
using namespace std::filesystem;

vector<string> parse_csv_line(const string& line, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream token_stream(line);
    bool in_quotes = false;
    while (std::getline(token_stream, token, delimiter)) {
        // Handle quoted fields that might contain delimiters
        if (!token.empty() && token.front() == '"') {
            if (token.back() == '"') {
                // Single cell quoted token
                token = token.substr(1, token.size() - 2);
            } else {
                // Multi-part quoted token
                in_quotes = true;
                token = token.substr(1);
                string rest;
                while (in_quotes && std::getline(token_stream, rest, delimiter)) {
                    token += delimiter + rest;
                    if (!rest.empty() && rest.back() == '"') {
                        token = token.substr(0, token.size() - 1);
                        in_quotes = false;
                    }
                }
            }
        }
        tokens.push_back(token);
    }
    return tokens;
}

std::expected<CSVData, std::string> parse_csv(std::filesystem::path path) {
    char delimiter = ',';
    CSVData data;
    ifstream file(path);
    if (!file.is_open()) {
        string error = "Error parsing csv: could not open file <" + path.string() + ">";
        return std::unexpected(error);
    }
    string line;
    if (std::getline(file, line))
        data.headers = parse_csv_line(line, delimiter);
    const size_t header_size = data.headers.size();
    while (std::getline(file, line)) {
        auto tokens = parse_csv_line(line, delimiter);
        if (tokens.size() != header_size) {
            string error = "Error parsing csv: inconsistent table size";
            return std::unexpected(error);
        }
        data.rows.push_back(tokens);
    }
    file.close();
    return data;
}