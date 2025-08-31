// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include <unordered_map>
#include <chrono>
#include <memory>

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
    bool isRegistered = false;
    std::chrono::steady_clock::time_point joinTime;
    bool isFrozen = false;
    std::shared_ptr<endstone::SchedulerTask> kickTask;
    std::shared_ptr<endstone::SchedulerTask> reminderTask;
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

    // New registration system methods
    static void freezePlayer(endstone::Player* pl);
    static void unfreezePlayer(endstone::Player* pl);
    static bool isPlayerFrozen(endstone::Player* pl);
    static void startRegistrationTimer(endstone::Player* pl);
    static void stopRegistrationTimer(endstone::Player* pl);
    static void kickUnregisteredPlayer(endstone::Player* pl);
    static void sendRegistrationReminder(endstone::Player* pl);
    static bool isPlayerRegistered(endstone::Player* pl);
    static void markPlayerAsRegistered(endstone::Player* pl);
    static std::chrono::seconds getTimeUntilKick(endstone::Player* pl);

private:
    static std::unordered_map<endstone::Player*, PlayerData> playerDataMap;
    static const std::chrono::seconds KICK_DELAY;
    static const std::chrono::seconds REMINDER_INTERVAL;
};

} // namespace PlayerRegister