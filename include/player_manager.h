// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include <unordered_map>

namespace PlayerRegister {

struct PlayerData {
    std::string id;
    std::string name;
    std::string password;
    int accounts = 0;

    endstone::UUID fakeUUID;
    std::string fakeXUID;
    std::string fakeDBkey;

    bool valid = false;
};

class PlayerManager {
public:
    static endstone::UUID getRealUUID(endstone::Player* pl);
    static endstone::UUID getFakeUUID(endstone::Player* pl);

    static std::string getFakeDBkey(const std::string& real);
    static void setFakeDBkey(endstone::Player* pl);
    static void setPlayerData(endstone::Player* pl, PlayerData& data);

    static void loadPlayer(endstone::Player* pl);
    static void unloadPlayer(endstone::Player* pl);

    static const PlayerData& getPlayerData(endstone::Player* pl);
    static endstone::Player* getPlayerByUUID(const endstone::UUID& uuid);
    static const std::unordered_map<endstone::Player*, PlayerData>& getAllData();
    static void clearAllData();
    static std::string getId(endstone::Player* pl);
    static void reconnect(endstone::Player* pl);

private:
    static std::unordered_map<endstone::Player*, PlayerData> playerDataMap;
};

} // namespace PlayerRegister