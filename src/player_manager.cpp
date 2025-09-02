// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_manager.h"

#include <endstone/endstone.hpp>
#include <thread>
#include <sstream>

namespace PlayerRegister {

std::unordered_map<endstone::Player*, PlayerData> PlayerManager::playerDataMap;
const std::chrono::seconds PlayerManager::KICK_DELAY = std::chrono::seconds(140); // 2 минуты 20 секунд
const std::chrono::seconds PlayerManager::REMINDER_INTERVAL = std::chrono::seconds(60); // 1 минута

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
    data.joinTime = std::chrono::steady_clock::now();
    data.isFrozen = false;
    data.kickTask = nullptr;
    data.reminderTask = nullptr;
    playerDataMap[pl] = data;
    
    // Start registration process
    freezePlayer(pl);
    startRegistrationTimer(pl);
}

void PlayerManager::unloadPlayer(endstone::Player* pl) {
    stopRegistrationTimer(pl);
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
        location.y = 256.0f; // High in the sky
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
        location.y = 64.0f; // Ground level
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
    
    // Create kick task using simple approach
    try {
        auto kickTaskId = pl->getServer().getScheduler().runTaskLater(
            [pl]() {
                kickUnregisteredPlayer(pl);
            },
            KICK_DELAY.count() * 20 // Convert to ticks (20 ticks = 1 second)
        );
        data.kickTask = std::make_shared<decltype(kickTaskId)>(kickTaskId);
    } catch (...) {
        // If scheduler fails, use fallback
        data.kickTask = nullptr;
    }
    
    // Create reminder task using simple approach
    try {
        auto reminderTaskId = pl->getServer().getScheduler().runTaskTimer(
            [pl]() {
                sendRegistrationReminder(pl);
            },
            REMINDER_INTERVAL.count() * 20, // Initial delay
            REMINDER_INTERVAL.count() * 20  // Repeat interval
        );
        data.reminderTask = std::make_shared<decltype(reminderTaskId)>(reminderTaskId);
    } catch (...) {
        // If scheduler fails, use fallback
        data.reminderTask = nullptr;
    }
}

void PlayerManager::stopRegistrationTimer(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Cancel kick task
    if (data.kickTask) {
        try {
            // Try to cancel the task - this is a simplified approach
            // In real implementation, you'd need to cast back to the proper type
            data.kickTask.reset();
        } catch (...) {
            // Ignore cancellation errors
        }
    }
    
    // Cancel reminder task
    if (data.reminderTask) {
        try {
            // Try to cancel the task - this is a simplified approach
            data.reminderTask.reset();
        } catch (...) {
            // Ignore cancellation errors
        }
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

} // namespace PlayerRegister