#ifndef CONFIG_H
#define CONFIG_H

#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

class Config {
    std::unordered_map<std::string, std::string> m_config_map;
    Config() = default;
    void add_key_value(std::string key, std::string value) { m_config_map[key] = value; }

  public:
    std::optional<std::string> get(std::string key) const {
        if (!m_config_map.contains(key))
            return std::nullopt;
        return m_config_map.at(key);
    }
    static std::expected<Config, std::string> create(std::filesystem::path path) {
        std::ifstream file(path);
        if (!file.is_open())
            return std::unexpected("Cannot open config file at" + path.string());
        std::string line;
        Config config;
        while (std::getline(file, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                config.add_key_value(key, value);
            }
        }
        return std::move(config);
    }
};

#endif