#include "config/config.hpp"

#include <fstream>
#include <stdexcept>
#include <unordered_map>

class ConfigImpl {
 public:
  std::unordered_map<std::string, std::string> m_config_map;
  ConfigImpl(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
      auto pos = line.find('=');
      if (pos != std::string::npos) {
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        m_config_map[key] = value;
      }
    }
  }
  const std::string& get(const std::string& key) const {
    if (!m_config_map.contains(key))
      throw std::runtime_error("Config: key not found" + key);
    return m_config_map.at(key);
  }
};

Config::Config(const std::string& filename) {
  m_config_impl = std::make_unique<ConfigImpl>(filename);
}

const std::string& Config::get(const std::string& key) const {
  return m_config_impl->get(key);
}

Config::~Config() = default;