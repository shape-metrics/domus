#ifndef CONFIG_H
#define CONFIG_H

#include <memory>
#include <string>

class ConfigImpl;

class Config {
    std::unique_ptr<ConfigImpl> m_config_impl;

  public:
    explicit Config(const std::string& filename);
    [[nodiscard]] const std::string& get(const std::string& key) const;
    ~Config();
};

#endif
