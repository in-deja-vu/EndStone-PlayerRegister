// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <string>

namespace PlayerRegister {

class Config {
public:
    int version = 5;
    std::string lang = "en_US";
    int max_accounts = 3;
    bool reconnect = false;
    std::string reconnect_ip = "127.0.0.1";
    unsigned short reconnect_port = 19132;
    bool fake_uuid = true;
    bool fake_xuid = true;

    static bool init(const std::string& configDir);
    static const Config& getInstance();

private:
    static Config instance;
    static bool loadConfig(const std::string& configPath);
    static bool saveConfig(const std::string& configPath);
};

#define CONF PlayerRegister::Config::getInstance()

} // namespace PlayerRegister