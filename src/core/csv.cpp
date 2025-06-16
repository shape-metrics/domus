#include "core/csv.hpp"

#include <fstream>
#include <sstream>

std::vector<std::string> parse_csv_line(const std::string& line,
                                        char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream token_stream(line);
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
        std::string rest;
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

CSVData parse_csv(const std::string& filename) {
  char delimiter = ',';
  CSVData data;
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("Could not open file: " + filename);
  std::string line;
  if (std::getline(file, line)) data.headers = parse_csv_line(line, delimiter);
  while (std::getline(file, line))
    data.rows.push_back(parse_csv_line(line, delimiter));
  file.close();
  return data;
}