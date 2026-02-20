#include "domus/core/csv.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

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

CSVData parse_csv(path path) {
    char delimiter = ',';
    CSVData data;
    ifstream file(path);
    if (!file.is_open())
        throw runtime_error("Could not open file: " + path.string());
    string line;
    if (std::getline(file, line))
        data.headers = parse_csv_line(line, delimiter);
    while (std::getline(file, line))
        data.rows.push_back(parse_csv_line(line, delimiter));
    file.close();
    return data;
}