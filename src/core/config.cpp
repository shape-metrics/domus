#include "domus/core/config.hpp"

#include <fstream>
#include <unordered_map>

namespace domus {

class ConfigImpl final : public Config {
    std::unordered_map<std::string, std::string> m_map;

  public:
    explicit ConfigImpl(std::unordered_map<std::string, std::string> map) : m_map(std::move(map)) {}

    std::optional<std::string> get(const std::string& key) const override {
        if (auto it = m_map.find(key); it != m_map.end())
            return it->second;
        return std::nullopt;
    }
};

std::expected<std::unique_ptr<Config>, std::string> Config::create(std::filesystem::path path) {
    return std::expected<std::filesystem::path, std::string>(path)
        .and_then([](const auto& p) -> std::expected<std::ifstream, std::string> {
            std::ifstream file(p);
            if (!file.is_open()) {
                return std::unexpected("Failed to open config file: " + p.string());
            }
            return file;
        })
        .transform([](std::ifstream file) {
            std::unordered_map<std::string, std::string> map;
            std::string line;
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

} // namespace domus