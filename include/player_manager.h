// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <optional>

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
    bool isAuthenticated = false;
    std::chrono::steady_clock::time_point joinTime;
    bool isFrozen = false;
    std::shared_ptr<endstone::Task> kickTask;
    std::shared_ptr<endstone::Task> reminderTask;
    
    // New fields for authorization system
    std::optional<endstone::Location> originalLocation;
    float originalYaw = 0.0f;
    float originalPitch = 0.0f;
    std::vector<std::unique_ptr<endstone::ItemStack>> savedInventory;
    std::shared_ptr<endstone::Task> authTimerTask;
    std::shared_ptr<endstone::Task> authReminderTask;
    
    // Default constructor
    PlayerData() = default;
    
    // Copy constructor
    PlayerData(const PlayerData& other) 
        : isRegistered(other.isRegistered)
        , isLoggedIn(other.isLoggedIn)
        , isFrozen(other.isFrozen)
        , registrationStartTime(other.registrationStartTime)
        , loginStartTime(other.loginStartTime)
        , authStartTime(other.authStartTime)
        , reminderTask(other.reminderTask)
        , originalLocation(other.originalLocation)
        , originalYaw(other.originalYaw)
        , originalPitch(other.originalPitch)
        , authTimerTask(other.authTimerTask)
        , authReminderTask(other.authReminderTask)
    {
        // Deep copy the inventory
        savedInventory.reserve(other.savedInventory.size());
        for (const auto& item : other.savedInventory) {
            if (item) {
                savedInventory.push_back(std::make_unique<endstone::ItemStack>(*item));
            }
        }
    }
    
    // Copy assignment operator
    PlayerData& operator=(const PlayerData& other) {
        if (this != &other) {
            isRegistered = other.isRegistered;
            isLoggedIn = other.isLoggedIn;
            isFrozen = other.isFrozen;
            registrationStartTime = other.registrationStartTime;
            loginStartTime = other.loginStartTime;
            authStartTime = other.authStartTime;
            reminderTask = other.reminderTask;
            originalLocation = other.originalLocation;
            originalYaw = other.originalYaw;
            originalPitch = other.originalPitch;
            authTimerTask = other.authTimerTask;
            authReminderTask = other.authReminderTask;
            
            // Deep copy the inventory
            savedInventory.clear();
            savedInventory.reserve(other.savedInventory.size());
            for (const auto& item : other.savedInventory) {
                if (item) {
                    savedInventory.push_back(std::make_unique<endstone::ItemStack>(*item));
                }
            }
        }
        return *this;
    }
    
    // Move constructor
    PlayerData(PlayerData&& other) noexcept = default;
    
    // Move assignment operator
    PlayerData& operator=(PlayerData&& other) noexcept = default;
};

class PlayerManager {
public:
    static void setPlugin(endstone::Plugin* plugin);
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
    
    // New authorization system methods
    static void startAuthorizationProcess(endstone::Player* pl);
    static void completeAuthorizationProcess(endstone::Player* pl);
    static void savePlayerState(endstone::Player* pl);
    static void restorePlayerState(endstone::Player* pl);
    static void startAuthorizationTimer(endstone::Player* pl);
    static void stopAuthorizationTimer(endstone::Player* pl);
    static void sendAuthorizationReminder(endstone::Player* pl, int secondsLeft);
    static bool isPlayerAuthenticated(endstone::Player* pl);
    static void markPlayerAsAuthenticated(endstone::Player* pl);
    static bool isCommandAllowed(const std::string& command);
    static bool isPlayerAuthorized(endstone::Player* pl);

private:
    static endstone::Plugin* plugin_;
    static std::unordered_map<endstone::Player*, PlayerData> playerDataMap;
    static const std::chrono::seconds KICK_DELAY;
    static const std::chrono::seconds REMINDER_INTERVAL;
    static const std::chrono::seconds AUTH_TIMEOUT;
    static const std::chrono::seconds AUTH_REMINDER_INTERVAL;
};

} // namespace PlayerRegister