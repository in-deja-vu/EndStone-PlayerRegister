// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "database.h"

#include <fstream>
#include <filesystem>
#include <endstone/logger.h>
#include <algorithm>

namespace PlayerRegister {

std::string Database::dataDir_;

bool Database::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    
    // Create directories if they don't exist
    std::string playersDir = dataDir_ + "/players";
    std::string accountsDir = dataDir_ + "/accounts";
    
    ensureDirectoryExists(playersDir);
    ensureDirectoryExists(accountsDir);
    
    return true;
}

std::string Database::getPlayerFilePath(const std::string& id) {
    return dataDir_ + "/players/" + id + ".json";
}

std::string Database::getAccountFilePath(const std::string& name) {
    return dataDir_ + "/accounts/" + name + ".json";
}

void Database::ensureDirectoryExists(const std::string& path) {
    std::filesystem::create_directories(path);
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
    // Parse UUID from string - Endstone UUID is a byte array
    std::string uuidStr = j["fakeUUID"].get<std::string>();
    // Remove hyphens from UUID string and parse as hex
    uuidStr.erase(std::remove(uuidStr.begin(), uuidStr.end(), '-'), uuidStr.end());
    if (uuidStr.length() == 32) {
        uint64_t high = std::stoull(uuidStr.substr(0, 16), nullptr, 16);
        uint64_t low = std::stoull(uuidStr.substr(16, 16), nullptr, 16);
        // Create UUID by filling the byte array
        for (int i = 0; i < 8; i++) {
            data.fakeUUID.data[i] = (high >> (8 * (7 - i))) & 0xFF;
            data.fakeUUID.data[8 + i] = (low >> (8 * (7 - i))) & 0xFF;
        }
    } else {
        // Fallback to a default UUID if parsing fails - all zeros
        for (int i = 0; i < 16; i++) {
            data.fakeUUID.data[i] = 0;
        }
    }
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
    return std::filesystem::remove(filePath);
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