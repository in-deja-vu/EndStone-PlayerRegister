// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "database.h"

#include <fstream>
#include <endstone/logger.h>
#include <algorithm>
#include <sstream>

namespace PlayerRegister {

std::string Database::dataDir_;

bool Database::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    
    // Create directories if they don't exist using simple string concatenation
    std::string playersPath = dataDir_ + "/players";
    std::string accountsPath = dataDir_ + "/accounts";
    
    ensureDirectoryExists(playersPath);
    ensureDirectoryExists(accountsPath);
    
    return true;
}

std::string Database::getPlayerFilePath(const std::string& id) {
    return dataDir_ + "/players/" + id + ".json";
}

std::string Database::getAccountFilePath(const std::string& name) {
    return dataDir_ + "/accounts/" + name + ".json";
}

void Database::ensureDirectoryExists(const std::string& path) {
    std::string dirCmd = "mkdir -p " + path;
    std::system(dirCmd.c_str());
}

nlohmann::json Database::serializeData(const PlayerData& data) {
    nlohmann::json j;
    j["name"] = data.name;
    j["password"] = data.password;
    j["accounts"] = data.accounts;
    j["fakeUUID"] = data.fakeUUID.str();
    j["fakeXUID"] = data.fakeXUID;
    j["fakeDBkey"] = data.fakeDBkey;
    return j;
}

void Database::deserializeData(const nlohmann::json& j, PlayerData& data) {
    data.name = j["name"].get<std::string>();
    data.password = j["password"].get<std::string>();
    data.accounts = j["accounts"].get<int>();
    // Parse UUID from string using shared function
    std::string uuidStr = j["fakeUUID"].get<std::string>();
    data.fakeUUID = PlayerManager::parseUUIDFromString(uuidStr);
    data.fakeXUID = j["fakeXUID"].get<std::string>();
    data.fakeDBkey = j["fakeDBkey"].get<std::string>();
}

void Database::storeAsPlayer(const PlayerData& data) {
    std::string filePath = getPlayerFilePath(data.id);
    nlohmann::json j = serializeData(data);
    
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << j.dump(4); // Pretty print with 4-space indentation
        file.close();
    }
}

void Database::loadAsPlayer(PlayerData& data) {
    std::string filePath = getPlayerFilePath(data.id);
    std::ifstream file(filePath);
    
    if (file.is_open()) {
        try {
            nlohmann::json j;
            file >> j;
            deserializeData(j, data);
            data.valid = true;
        } catch (const nlohmann::json::exception& e) {
            // Invalid JSON, keep data.valid = false
        }
        file.close();
    }
}

bool Database::removePlayer(const std::string& id) {
    std::string filePath = getPlayerFilePath(id);
    std::string rmCmd = "rm -f " + filePath;
    return std::system(rmCmd.c_str()) == 0;
}

void Database::storeAsAccount(const PlayerData& data) {
    std::string filePath = getAccountFilePath(data.name);
    nlohmann::json j = serializeData(data);
    
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << j.dump(4); // Pretty print with 4-space indentation
        file.close();
    }
}

void Database::loadAsAccount(PlayerData& data) {
    std::string filePath = getAccountFilePath(data.name);
    std::ifstream file(filePath);
    
    if (file.is_open()) {
        try {
            nlohmann::json j;
            file >> j;
            deserializeData(j, data);
            data.valid = true;
        } catch (const nlohmann::json::exception& e) {
            // Invalid JSON, keep data.valid = false
        }
        file.close();
    }
}

} // namespace PlayerRegister