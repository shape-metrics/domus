#include "domus/config/config.hpp"

#include <fstream>
#include <stdexcept>
#include <unordered_map>

using namespace std;

class ConfigImpl {
  public:
    unordered_map<string, string> m_config_map;
    explicit ConfigImpl(const string& filename) {
        std::ifstream file(filename);
        string line;
        if (!file.is_open())
            throw runtime_error("Config: Could not open file " + filename);
        while (std::getline(file, line)) {
            auto pos = line.find('=');
            if (pos != string::npos) {
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                m_config_map[key] = value;
            }
        }
    }
    const string& get(const string& key) const {
        if (!m_config_map.contains(key))
            throw runtime_error("Config: key not found " + key);
        return m_config_map.at(key);
    }
};

Config::Config(const string& filename) { m_config_impl = make_unique<ConfigImpl>(filename); }

const string& Config::get(const string& key) const { return m_config_impl->get(key); }

Config::~Config() = default;