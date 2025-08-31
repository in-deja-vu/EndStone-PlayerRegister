// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_manager.h"

#include <endstone/endstone.hpp>

namespace PlayerRegister {

std::unordered_map<endstone::Player*, PlayerData> PlayerManager::playerDataMap;

endstone::UUID PlayerManager::getRealUUID(endstone::Player* pl) {
    return pl->getUniqueId();
}

endstone::UUID PlayerManager::getFakeUUID(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end() && it->second.valid) {
        return it->second.fakeUUID;
    }
    return pl->getUniqueId();
}

std::string PlayerManager::getFakeDBkey(const std::string& real) {
    // In Endstone, we might not need the same DB key system as LeviLamina
    // For now, return a modified version
    return "player_server_" + real;
}

void PlayerManager::setFakeDBkey(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        it->second.fakeDBkey = getFakeDBkey(pl->getUniqueId().str());
    }
}

void PlayerManager::setPlayerData(endstone::Player* pl, PlayerData& data) {
    playerDataMap[pl] = data;
}

void PlayerManager::loadPlayer(endstone::Player* pl) {
    // Initialize player data when they join
    PlayerData data;
    data.id = getId(pl);
    data.valid = false;
    playerDataMap[pl] = data;
}

void PlayerManager::unloadPlayer(endstone::Player* pl) {
    playerDataMap.erase(pl);
}

const PlayerData& PlayerManager::getPlayerData(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        return it->second;
    }
    static PlayerData emptyData;
    return emptyData;
}

endstone::Player* PlayerManager::getPlayerByUUID(const endstone::UUID& uuid) {
    // This would require access to the server's player list
    // For now, return nullptr as this is a simplified implementation
    return nullptr;
}

const std::unordered_map<endstone::Player*, PlayerData>& PlayerManager::getAllData() {
    return playerDataMap;
}

void PlayerManager::clearAllData() {
    playerDataMap.clear();
}

std::string PlayerManager::getId(endstone::Player* pl) {
    return pl->getUniqueId().str();
}

void PlayerManager::reconnect(endstone::Player* pl) {
    // In Endstone, we can't force a player to reconnect like in LeviLamina
    // Instead, we'll just send a message asking them to reconnect
    pl->sendMessage(endstone::ColorFormat::Yellow + "Please reconnect to the server to complete the login process.");
}

} // namespace PlayerRegister