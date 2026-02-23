#include "domus/core/config.hpp"

#include <fstream>
#include <unordered_map>

using namespace std;

class ConfigImpl final : public Config {
    unordered_map<string, string> m_map;

  public:
    explicit ConfigImpl(unordered_map<string, string> map) : m_map(std::move(map)) {}

    optional<string> get(string_view key) const override {
        if (auto it = m_map.find(std::string(key)); it != m_map.end())
            return it->second;
        return std::nullopt;
    }
};

expected<unique_ptr<Config>, string> Config::create(filesystem::path path) {
    return std::expected<filesystem::path, string>(path)
        .and_then([](const auto& p) -> std::expected<std::ifstream, std::string> {
            std::ifstream file(p);
            if (!file.is_open()) {
                return std::unexpected("Failed to open config file: " + p.string());
            }
            return file;
        })
        .transform([](std::ifstream file) {
            unordered_map<string, string> map;
            string line;
            while (std::getline(file, line)) {
                if (auto pos = line.find('='); pos != std::string::npos) {
                    auto key = line.substr(0, pos);
                    auto value = line.substr(pos + 1);
                    map[std::move(key)] = std::move(value);
                }
            }
            return std::make_unique<ConfigImpl>(std::move(map));
        });
}