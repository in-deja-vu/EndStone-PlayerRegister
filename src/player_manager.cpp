// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_manager.h"

#include <endstone/endstone.hpp>
#include <thread>
#include <sstream>
#include <algorithm>

namespace PlayerRegister {

std::unordered_map<endstone::Player*, PlayerData> PlayerManager::playerDataMap;
endstone::Plugin* PlayerManager::plugin_ = nullptr;
const std::chrono::seconds PlayerManager::KICK_DELAY = std::chrono::seconds(140); // 2 минуты 20 секунд
const std::chrono::seconds PlayerManager::REMINDER_INTERVAL = std::chrono::seconds(60); // 1 минута
const std::chrono::seconds PlayerManager::AUTH_TIMEOUT = std::chrono::seconds(60); // 60 секунд
const std::chrono::seconds PlayerManager::AUTH_REMINDER_INTERVAL = std::chrono::seconds(15); // 15 секунд

// Helper function to parse UUID from string
endstone::UUID PlayerManager::parseUUIDFromString(const std::string& uuidStr) {
    endstone::UUID result;
    
    // Remove hyphens from UUID string and parse as hex
    std::string cleanUuidStr = uuidStr;
    cleanUuidStr.erase(std::remove(cleanUuidStr.begin(), cleanUuidStr.end(), '-'), cleanUuidStr.end());
    
    if (cleanUuidStr.length() == 32) {
        uint64_t high = std::stoull(cleanUuidStr.substr(0, 16), nullptr, 16);
        uint64_t low = std::stoull(cleanUuidStr.substr(16, 16), nullptr, 16);
        // Create UUID by filling the byte array
        for (int i = 0; i < 8; i++) {
            result.data[i] = (high >> (8 * (7 - i))) & 0xFF;
            result.data[8 + i] = (low >> (8 * (7 - i))) & 0xFF;
        }
    } else {
        // Fallback to a default UUID if parsing fails - all zeros
        for (int i = 0; i < 16; i++) {
            result.data[i] = 0;
        }
    }
    
    return result;
}

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
        endstone::Location location = pl->getLocation();
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
        endstone::Location location = pl->getLocation();
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
                    auto* player = server.getPlayer(PlayerManager::parseUUIDFromString(playerId));
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
                    auto* player = server.getPlayer(PlayerManager::parseUUIDFromString(playerId));
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
    
    // Debug output - show initial location
    if (plugin_) {
        endstone::Location initialLocation = pl->getLocation();
        std::ostringstream msg;
        msg << "Starting authorization process for " << pl->getName() << ": initial location=(" 
            << initialLocation.getX() << "," << initialLocation.getY() << "," << initialLocation.getZ() << ")";
        plugin_->getLogger().info(msg.str());
    }
    
    // Save player state FIRST - this saves the original spawn location
    savePlayerState(pl);
    
    // Clear inventory
    auto& inventory = pl->getInventory();
    for (int i = 0; i < inventory.getSize(); i++) {
        inventory.clear(i);
    }
    
    // Teleport player to height 15000 for authorization
    endstone::Location location = pl->getLocation();
    location.setY(15000.0f);
    pl->teleport(location);
    
    // Debug output - confirm teleportation
    if (plugin_) {
        endstone::Location newLocation = pl->getLocation();
        std::ostringstream msg;
        msg << "Player " << pl->getName() << " teleported to authorization area: (" 
            << newLocation.getX() << "," << newLocation.getY() << "," << newLocation.getZ() << ")";
        plugin_->getLogger().info(msg.str());
    }
    
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
    
    // Debug output
    if (plugin_) {
        endstone::Location currentLocation = pl->getLocation();
        std::ostringstream msg;
        msg << "Completing authorization process for " << pl->getName() << ": current location=(" 
            << currentLocation.getX() << "," << currentLocation.getY() << "," << currentLocation.getZ() << ")";
        plugin_->getLogger().info(msg.str());
    }
    
    // Stop authorization timer
    stopAuthorizationTimer(pl);
    
    // Restore player state - this should teleport player back to original location
    restorePlayerState(pl);
    
    // Mark player as authenticated
    markPlayerAsAuthenticated(pl);
    
    // Send welcome message
    pl->sendMessage(endstone::ColorFormat::Green + "Вы успешно авторизованы! Добро пожаловать на сервер!");
    
    // Final debug output
    if (plugin_) {
        endstone::Location finalLocation = pl->getLocation();
        std::ostringstream msg;
        msg << "Authorization completed for " << pl->getName() << ": final location=(" 
            << finalLocation.getX() << "," << finalLocation.getY() << "," << finalLocation.getZ() << ")";
        plugin_->getLogger().info(msg.str());
    }
}

void PlayerManager::savePlayerState(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Save original location and rotation BEFORE any teleportation
    endstone::Location currentLocation = pl->getLocation();
    data.originalLocation = std::make_unique<endstone::Location>(currentLocation);
    data.originalYaw = currentLocation.getYaw();
    data.originalPitch = currentLocation.getPitch();
    
    // Debug output
    if (plugin_) {
        std::ostringstream msg;
        msg << "Saving player state for " << pl->getName() << ": location=(" 
            << currentLocation.getX() << "," << currentLocation.getY() << "," << currentLocation.getZ() 
            << "), yaw=" << currentLocation.getYaw() << ", pitch=" << currentLocation.getPitch();
        plugin_->getLogger().info(msg.str());
    }
    
    // Save inventory
    data.savedInventory.clear();
    auto& inventory = pl->getInventory();
    for (int i = 0; i < inventory.getSize(); i++) {
        auto item = inventory.getItem(i);
        if (item) {
            // Clone the item and store it as unique_ptr
            data.savedInventory.push_back(std::make_unique<endstone::ItemStack>(*item));
        }
    }
}

void PlayerManager::restorePlayerState(endstone::Player* pl) {
    auto it = playerDataMap.find(pl);
    if (it == playerDataMap.end()) return;
    
    auto& data = it->second;
    
    // Debug output
    if (plugin_) {
        endstone::Location currentLocation = pl->getLocation();
        std::ostringstream msg;
        msg << "Restoring player state for " << pl->getName() << ": current location=(" 
            << currentLocation.getX() << "," << currentLocation.getY() << "," << currentLocation.getZ() 
            << "), yaw=" << currentLocation.getYaw() << ", pitch=" << currentLocation.getPitch();
        plugin_->getLogger().info(msg.str());
    }
    
    // Restore location and rotation if available
    if (data.originalLocation) {
        auto& originalLoc = *data.originalLocation;
        
        // Debug output
        if (plugin_) {
            std::ostringstream msg;
            msg << "Teleporting player " << pl->getName() << " back to original location: (" 
                << originalLoc.getX() << "," << originalLoc.getY() << "," << originalLoc.getZ() 
                << "), yaw=" << data.originalYaw << ", pitch=" << data.originalPitch;
            plugin_->getLogger().info(msg.str());
        }
        
        // Create a new location with the original coordinates and rotation
        // Use the dimension from the original location, fallback to current if not available
        auto* dimension = originalLoc.getDimension();
        if (!dimension) {
            dimension = pl->getLocation().getDimension();
        }
        
        if (dimension) {
            // Create location with dimension - correct constructor order
            endstone::Location restoreLocation(
                dimension,               // Dimension pointer (first argument)
                originalLoc.getX(),      // X coordinate
                originalLoc.getY(),      // Y coordinate
                originalLoc.getZ(),      // Z coordinate
                data.originalYaw,        // Yaw (horizontal rotation)
                data.originalPitch       // Pitch (vertical rotation)
            );
            pl->teleport(restoreLocation);
        } else {
            // Fallback: create location without dimension - use nullptr for dimension
            endstone::Location restoreLocation(
                nullptr,                 // No dimension
                originalLoc.getX(),      // X coordinate
                originalLoc.getY(),      // Y coordinate
                originalLoc.getZ(),      // Z coordinate
                data.originalYaw,        // Yaw (horizontal rotation)
                data.originalPitch       // Pitch (vertical rotation)
            );
            pl->teleport(restoreLocation);
        }
        
        // Additional debug output
        if (plugin_) {
            endstone::Location newLocation = pl->getLocation();
            std::ostringstream msg;
            msg << "Player " << pl->getName() << " teleported successfully to: (" 
                << newLocation.getX() << "," << newLocation.getY() << "," << newLocation.getZ() 
                << "), yaw=" << newLocation.getYaw() << ", pitch=" << newLocation.getPitch();
            plugin_->getLogger().info(msg.str());
        }
    } else {
        // Fallback: teleport to spawn location if no original location saved
        if (plugin_) {
            std::ostringstream msg;
            msg << "No original location saved for player " << pl->getName() << ", teleporting to world spawn";
            plugin_->getLogger().info(msg.str());
        }
        
        // Create a simple spawn location at ground level with current position
        endstone::Location currentLocation = pl->getLocation();
        endstone::Location spawnLocation(
            currentLocation.getDimension(),  // Dimension pointer
            currentLocation.getX(),           // X coordinate
            currentLocation.getY(),           // Y coordinate (use current Y instead of hardcoded 64)
            currentLocation.getZ(),           // Z coordinate
            0.0f,                            // Yaw (horizontal rotation)
            0.0f                             // Pitch (vertical rotation)
        );
        pl->teleport(spawnLocation);
        
        if (plugin_) {
            std::ostringstream msg;
            msg << "Player " << pl->getName() << " teleported to fallback spawn: (" 
                << spawnLocation.getX() << "," << spawnLocation.getY() << "," << spawnLocation.getZ() << ")";
            plugin_->getLogger().info(msg.str());
        }
    }
    
    // Restore inventory
    auto& inventory = pl->getInventory();
    inventory.clear();
    
    // Create a vector of pointers for addItem
    std::vector<endstone::ItemStack*> itemPointers;
    itemPointers.reserve(data.savedInventory.size());
    for (auto& item : data.savedInventory) {
        itemPointers.push_back(item.get());
    }
    
    // Add all items at once
    if (!itemPointers.empty()) {
        inventory.addItem(itemPointers);
        
        if (plugin_) {
            std::ostringstream msg;
            msg << "Restored " << itemPointers.size() << " items for player " << pl->getName();
            plugin_->getLogger().info(msg.str());
        }
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
                    auto* player = server.getPlayer(PlayerManager::parseUUIDFromString(playerId));
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
                    auto* player = server.getPlayer(PlayerManager::parseUUIDFromString(playerId));
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