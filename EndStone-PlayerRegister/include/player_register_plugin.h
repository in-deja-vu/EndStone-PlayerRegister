// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <string>

#include "account_manager.h"
#include "player_data.h"
#include "auth_listener.h"
#include "auth_command_executor.h"

class PlayerRegisterPlugin : public endstone::Plugin {
public:
    void onLoad() override;
    void onEnable() override;
    void onDisable() override;
    
    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, 
                   const std::vector<std::string> &args) override;
    
    // Event handlers
    void onPlayerJoin(endstone::PlayerJoinEvent &event);
    void onPlayerQuit(endstone::PlayerQuitEvent &event);
    void onPlayerChat(endstone::PlayerChatEvent &event);
    void onServerLoad(endstone::ServerLoadEvent &event);
    
    // Player management methods
    void savePlayerData(endstone::Player &player);
    void applyAuthEffects(endstone::Player &player);
    void teleportToAuthPosition(endstone::Player &player);
    void restorePlayerState(endstone::Player &player);
    void completeAuthentication(endstone::Player &player);
    
    // Timer management
    void startAuthTimer(endstone::Player &player);
    void stopAuthTimer(endstone::Player &player);
    void sendAuthReminder(endstone::Player &player);
    void kickUnauthedPlayer(endstone::Player &player);
    
    // Auth state management
    bool isPlayerAuthenticated(endstone::Player *player);
    void setPlayerAuthenticated(endstone::Player &player, bool authenticated);
    
    // Chat and command filtering
    bool shouldAllowChat(endstone::Player &player);
    bool shouldAllowCommand(endstone::Player &player, const std::string &command);

private:
    std::unique_ptr<AuthListener> listener_;
    std::unique_ptr<AuthCommandExecutor> commandExecutor_;
    
    // Player data storage
    std::unordered_map<endstone::Player*, PlayerData> playerDataMap_;
    
    // Constants
    static constexpr std::chrono::seconds AUTH_TIMEOUT{150}; // 150 seconds
    static constexpr std::chrono::seconds REMINDER_INTERVAL{30}; // 30 seconds
    static constexpr int EFFECT_AMPLIFIER = 254; // 255 level (0-indexed)
    static constexpr int EFFECT_DURATION = 999999; // Very long duration
};