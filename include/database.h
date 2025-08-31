// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include "player_manager.h"

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

namespace PlayerRegister {

class Database {
public:
    static bool init(const std::string& dataDir);

    static void storeAsPlayer(const PlayerData& data);
    static void loadAsPlayer(PlayerData& data);
    static bool removePlayer(const std::string& id);

    static void storeAsAccount(const PlayerData& data);
    static void loadAsAccount(PlayerData& data);

private:
    static std::string dataDir_;
    static std::string getPlayerFilePath(const std::string& id);
    static std::string getAccountFilePath(const std::string& name);
    static void ensureDirectoryExists(const std::string& path);
    static nlohmann::json serializeData(const PlayerData& data);
    static void deserializeData(const nlohmann::json& j, PlayerData& data);
};

} // namespace PlayerRegister