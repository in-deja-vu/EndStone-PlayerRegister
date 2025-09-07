// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_manager.h"

#include <endstone/endstone.hpp>
#include <thread>
#include <sstream>

namespace PlayerRegister {

std::unordered_map<endstone::Player*, PlayerData> PlayerManager::playerDataMap;
endstone::Plugin* PlayerManager::plugin_ = nullptr;
const std::chrono::seconds PlayerManager::KICK_DELAY = std::chrono::seconds(140); // 2 минуты 20 секунд
const std::chrono::seconds PlayerManager::REMINDER_INTERVAL = std::chrono::seconds(60); // 1 минута
const std::chrono::seconds PlayerManager::AUTH_TIMEOUT = std::chrono::seconds(60); // 60 секунд
const std::chrono::seconds PlayerManager::AUTH_REMINDER_INTERVAL = std::chrono::seconds(15); // 15 секунд

void PlayerManager::setPlugin(endstone::Plugin* plugin) {
    plugin_ = plugin;
}

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
    data.isRegistered = false;
    data.isAuthenticated = false;
    data.joinTime = std::chrono::steady_clock::now();
    data.isFrozen = false;
    playerDataMap[pl] = data;
    
    // Start authorization process
    startAuthorizationProcess(pl);
}

void PlayerManager::unloadPlayer(endstone::Player* pl) {
    stopRegistrationTimer(pl);
    stopAuthorizationTimer(pl);
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
    for (auto& pair : playerDataMap) {
        stopRegistrationTimer(pair.first);
        stopAuthorizationTimer(pair.first);
    }
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

// New registration system implementation

void PlayerManager::freezePlayer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        it->second.isFrozen = true;
        
        // Set player as unable to move
        pl->setAllowFlight(false);
        pl->setFlying(false);
        pl->setWalkSpeed(0.0f);
        pl->setFlySpeed(0.0f);
        
        // Teleport player to a high location (in the sky)
        auto location = pl->getLocation();
        location.setY(256.0f); // High in the sky
        pl->teleport(location);
        
        pl->sendMessage(endstone::ColorFormat::Red + "Вы заморожены! Пожалуйста, зарегистрируйтесь чтобы играть.");
        pl->sendMessage(endstone::ColorFormat::Gold + "Используйте /register <ник> <пароль> <подтверждение> для регистрации");
        pl->sendMessage(endstone::ColorFormat::Gold + "Или /login <ник> <пароль> для входа в существующий аккаунт");
    }
}

void PlayerManager::unfreezePlayer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        it->second.isFrozen = false;
        
        // Restore normal movement
        pl->setWalkSpeed(0.2f);
        pl->setFlySpeed(0.1f);
        
        // Teleport player to ground level
        auto location = pl->getLocation();
        location.setY(64.0f); // Ground level
        pl->teleport(location);
        
        pl->sendMessage(endstone::ColorFormat::Green + "Вы успешно разморожены! Добро пожаловать на сервер!");
    }
}

bool PlayerManager::isPlayerFrozen(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    return it != playerDataMap.end() && it->second.isFrozen;
}

void PlayerManager::startRegistrationTimer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Stop any existing timers
    stopRegistrationTimer(pl);
    
    // Create kick task
    if (plugin_) {
        std::string playerId = pl->getUniqueId().str();
        data.kickTask = plugin_->getServer().getScheduler().runTaskLater(
            *plugin_,
            [playerId]() {
                // Find player by ID instead of using raw pointer
                if (plugin_) {
                    auto& server = plugin_->getServer();
                    auto* player = server.getPlayer(endstone::UUID::fromString(playerId));
                    if (player) {
                        kickUnregisteredPlayer(player);
                    }
                }
            },
            KICK_DELAY.count() * 20 // Convert to ticks (20 ticks = 1 second)
        );
        
        // Create reminder task
        data.reminderTask = plugin_->getServer().getScheduler().runTaskTimer(
            *plugin_,
            [playerId]() {
                // Find player by ID instead of using raw pointer
                if (plugin_) {
                    auto& server = plugin_->getServer();
                    auto* player = server.getPlayer(endstone::UUID::fromString(playerId));
                    if (player) {
                        sendRegistrationReminder(player);
                    }
                }
            },
            REMINDER_INTERVAL.count() * 20, // Initial delay
            REMINDER_INTERVAL.count() * 20  // Repeat interval
        );
    }
}

void PlayerManager::stopRegistrationTimer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Cancel kick task
    if (data.kickTask) {
        data.kickTask->cancel();
        data.kickTask.reset();
    }
    
    // Cancel reminder task
    if (data.reminderTask) {
        data.reminderTask->cancel();
        data.reminderTask.reset();
    }
}

void PlayerManager::kickUnregisteredPlayer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    if (!data.isRegistered) {
        pl->kick(endstone::ColorFormat::Red + "Вы были кикнуты за то, что не зарегистрировались в течение 2 минут 20 секунд!");
    }
}

void PlayerManager::sendRegistrationReminder(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    if (!data.isRegistered) {
        auto timeLeft = getTimeUntilKick(pl);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(timeLeft).count();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeLeft % std::chrono::minutes(1)).count();
        
        std::ostringstream msg;
        msg << endstone::ColorFormat::Yellow << "Пожалуйста, зарегистрируйтесь! У вас осталось " 
            << minutes << " минут " << seconds << " секунд.";
        
        pl->sendMessage(msg.str());
        pl->sendMessage(endstone::ColorFormat::Gold + "/register <ник> <пароль> <подтверждение> или /login <ник> <пароль>");
    }
}

bool PlayerManager::isPlayerRegistered(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    return it != playerDataMap.end() && it->second.isRegistered;
}

void PlayerManager::markPlayerAsRegistered(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        it->second.isRegistered = true;
        stopRegistrationTimer(pl);
        unfreezePlayer(pl);
    }
}

std::chrono::seconds PlayerManager::getTimeUntilKick(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return std::chrono::seconds(0);
    
    auto& data = it->second;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - data.joinTime;
    auto remaining = KICK_DELAY - elapsed;
    
    if (remaining.count() < 0) {
        return std::chrono::seconds(0);
    }
    
    return std::chrono::duration_cast<std::chrono::seconds>(remaining);
}

// New authorization system implementation

void PlayerManager::startAuthorizationProcess(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Save player state
    savePlayerState(pl);
    
    // Clear inventory
    auto& inventory = pl->getInventory();
    for (int i = 0; i < inventory.getSize(); i++) {
        inventory.clear(i);
    }
    
    // Teleport player to height 15000
    auto location = pl->getLocation();
    location.setY(15000.0f);
    pl->teleport(location);
    
    // Send title message
    pl->sendTitle("Пожалуйста, зарегистрируйтесь", "для продолжения игры", 10, 120, 20);
    
    // Start authorization timer
    startAuthorizationTimer(pl);
    
    // Send initial message
    pl->sendMessage(endstone::ColorFormat::Yellow + "Добро пожаловать на сервер!");
    pl->sendMessage(endstone::ColorFormat::Gold + "Пожалуйста, зарегистрируйтесь или войдите в аккаунт чтобы играть.");
    pl->sendMessage(endstone::ColorFormat::Gold + "Используйте /register <пароль> <подтверждение> для регистрации");
    pl->sendMessage(endstone::ColorFormat::Gold + "Или /login <пароль> для входа в существующий аккаунт");
}

void PlayerManager::completeAuthorizationProcess(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    // Stop authorization timer
    stopAuthorizationTimer(pl);
    
    // Restore player state
    restorePlayerState(pl);
    
    // Mark player as authenticated
    markPlayerAsAuthenticated(pl);
    
    // Send welcome message
    pl->sendMessage(endstone::ColorFormat::Green + "Вы успешно авторизованы! Добро пожаловать на сервер!");
}

void PlayerManager::savePlayerState(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Save original location and rotation
    data.originalLocation = pl->getLocation();
    data.originalYaw = pl->getYaw();
    data.originalPitch = pl->getPitch();
    
    // Save inventory
    data.savedInventory.clear();
    auto& inventory = pl->getInventory();
    for (int i = 0; i < inventory.getSize(); i++) {
        auto item = inventory.getItem(i);
        if (item) {
            data.savedInventory.push_back(*item);
        }
    }
}

void PlayerManager::restorePlayerState(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Restore location and rotation if available
    if (data.originalLocation) {
        pl->teleport(*data.originalLocation);
        pl->setYaw(data.originalYaw);
        pl->setPitch(data.originalPitch);
    }
    
    // Restore inventory
    auto& inventory = pl->getInventory();
    inventory.clear();
    
    for (const auto& item : data.savedInventory) {
        inventory.addItem(item);
    }
    
    // Clear saved inventory
    data.savedInventory.clear();
    data.originalLocation.reset();
}

void PlayerManager::startAuthorizationTimer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Stop any existing timers
    stopAuthorizationTimer(pl);
    
    if (plugin_) {
        std::string playerId = pl->getUniqueId().str();
        // Create kick task
        data.authTimerTask = plugin_->getServer().getScheduler().runTaskLater(
            *plugin_,
            [playerId]() {
                if (plugin_) {
                    auto& server = plugin_->getServer();
                    auto* player = server.getPlayer(endstone::UUID::fromString(playerId));
                    if (player && !PlayerManager::isPlayerAuthenticated(player)) {
                        player->kick(endstone::ColorFormat::Red + "Время авторизации истекло");
                    }
                }
            },
            AUTH_TIMEOUT.count() * 20 // Convert to ticks (20 ticks = 1 second)
        );
        
        // Create reminder task with specific intervals
        data.authReminderTask = plugin_->getServer().getScheduler().runTaskTimer(
            *plugin_,
            [playerId]() {
                if (plugin_) {
                    auto& server = plugin_->getServer();
                    auto* player = server.getPlayer(endstone::UUID::fromString(playerId));
                    if (player && !PlayerManager::isPlayerAuthenticated(player)) {
                        auto it = PlayerManager::playerDataMap.find(player);
                        if (it != PlayerManager::playerDataMap.end()) {
                            auto& data = it->second;
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - data.joinTime);
                            auto timeLeft = AUTH_TIMEOUT.count() - elapsed.count();
                            
                            if (timeLeft > 0) {
                                if (timeLeft == 45 || timeLeft == 30 || timeLeft == 15) {
                                    PlayerManager::sendAuthorizationReminder(player, static_cast<int>(timeLeft));
                                }
                            }
                        }
                    }
                }
            },
            15 * 20, // Start checking after 15 seconds
            15 * 20  // Check every 15 seconds
        );
    }
}

void PlayerManager::stopAuthorizationTimer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Cancel kick task
    if (data.authTimerTask) {
        data.authTimerTask->cancel();
        data.authTimerTask.reset();
    }
    
    // Cancel reminder task
    if (data.authReminderTask) {
        data.authReminderTask->cancel();
        data.authReminderTask.reset();
    }
}

void PlayerManager::sendAuthorizationReminder(endstone::Player* pl, int secondsLeft) {
    if (!pl) return;
    
    // Send chat message
    std::ostringstream msg;
    msg << endstone::ColorFormat::Yellow << "[Auth] Осталось " << secondsLeft << " секунд для авторизации.";
    pl->sendMessage(msg.str());
    
    // Send title/subtitle
    pl->sendTitle("Время авторизации истекает!", "Осталось: " + std::to_string(secondsLeft) + " секунд", 0, 40, 10);
}

bool PlayerManager::isPlayerAuthenticated(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    return it != playerDataMap.end() && it->second.isAuthenticated;
}

void PlayerManager::markPlayerAsAuthenticated(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it != playerDataMap.end()) {
        it->second.isAuthenticated = true;
    }
}

bool PlayerManager::isCommandAllowed(const std::string& command) {
    // Allow only register and login commands for unauthorized players
    return command == "register" || command == "login";
}

bool PlayerManager::isPlayerAuthorized(endstone::Player* pl) {
    return isPlayerAuthenticated(pl);
}

} // namespace PlayerRegister