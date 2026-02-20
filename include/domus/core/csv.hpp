#ifndef CSV_HANDLER_H
#define CSV_HANDLER_H

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

struct CSVData {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

std::expected<CSVData, std::string> parse_csv(std::filesystem::path path);

#endif