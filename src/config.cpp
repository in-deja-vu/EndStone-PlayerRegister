// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "config.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace PlayerRegister {

Config Config::instance;

bool Config::init(const std::string& configDir) {
    std::string configPath = configDir + "/config.json";
    
    // Create config directory if it doesn't exist - cross-platform solution
#ifdef _WIN32
    std::string dirCmd = "mkdir \"" + configDir + "\" 2>nul";
#else
    std::string dirCmd = "mkdir -p \"" + configDir + "\" 2>/dev/null";
#endif
    std::system(dirCmd.c_str());
    
    if (!loadConfig(configPath)) {
        return saveConfig(configPath);
    }
    
    return true;
}

bool Config::loadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;
        
        if (j.contains("version")) instance.version = j["version"].get<int>();
        if (j.contains("lang")) instance.lang = j["lang"].get<std::string>();
        if (j.contains("max_accounts")) instance.max_accounts = j["max_accounts"].get<int>();
        if (j.contains("reconnect")) instance.reconnect = j["reconnect"].get<bool>();
        if (j.contains("reconnect_ip")) instance.reconnect_ip = j["reconnect_ip"].get<std::string>();
        if (j.contains("reconnect_port")) instance.reconnect_port = j["reconnect_port"].get<unsigned short>();
        if (j.contains("fake_uuid")) instance.fake_uuid = j["fake_uuid"].get<bool>();
        if (j.contains("fake_xuid")) instance.fake_xuid = j["fake_xuid"].get<bool>();
        
    } catch (const nlohmann::json::exception& e) {
        return false;
    }
    
    file.close();
    return true;
}

bool Config::saveConfig(const std::string& configPath) {
    nlohmann::json j;
    j["version"] = instance.version;
    j["lang"] = instance.lang;
    j["max_accounts"] = instance.max_accounts;
    j["reconnect"] = instance.reconnect;
    j["reconnect_ip"] = instance.reconnect_ip;
    j["reconnect_port"] = instance.reconnect_port;
    j["fake_uuid"] = instance.fake_uuid;
    j["fake_xuid"] = instance.fake_xuid;
    
    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    file << j.dump(4); // Pretty print with 4-space indentation
    file.close();
    
    return true;
}

const Config& Config::getInstance() {
    return instance;
}

} // namespace PlayerRegister