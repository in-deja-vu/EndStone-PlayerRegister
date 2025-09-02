// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_register_plugin.h"
#include "account_manager.h"
#include <sstream>
#include <algorithm>

void PlayerRegisterPlugin::onLoad() {
    getLogger().info("PlayerRegister plugin loading...");
    
    // Initialize account manager
    if (!PlayerRegister::AccountManager::init(getDataFolder().string())) {
        getLogger().error("Failed to initialize account manager!");
        return;
    }
    
    getLogger().info("Account manager initialized successfully.");
}

void PlayerRegisterPlugin::onEnable() {
    getLogger().info("PlayerRegister plugin enabled!");
    
    // Initialize command executor
    commandExecutor_ = std::make_unique<AuthCommandExecutor>(*this);
    
    // Set up command executors
    if (auto *command = getCommand("register")) {
        command->setExecutor(std::make_unique<AuthCommandExecutor>(*this));
    }
    
    if (auto *command = getCommand("login")) {
        command->setExecutor(std::make_unique<AuthCommandExecutor>(*this));
    }
    
    // Initialize event listener
    listener_ = std::make_unique<AuthListener>(*this);
    
    // Register event handlers
    registerEvent(&PlayerRegisterPlugin::onPlayerJoin, *this);
    registerEvent(&PlayerRegisterPlugin::onPlayerQuit, *this);
    registerEvent(&PlayerRegisterPlugin::onPlayerChat, *this);
    registerEvent(&PlayerRegisterPlugin::onServerLoad, *this);
    
    getLogger().info("All commands and events registered successfully!");
}

void PlayerRegisterPlugin::onDisable() {
    getLogger().info("PlayerRegister plugin disabled!");
    
    // Clean up all auth timers
    for (auto& pair : playerDataMap_) {
        auto& data = pair.second;
        if (data.authTimerTask) {
            data.authTimerTask->cancel();
        }
        if (data.reminderTask) {
            data.reminderTask->cancel();
        }
    }
    playerDataMap_.clear();
}

bool PlayerRegisterPlugin::onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                                   const std::vector<std::string> &args) {
    return commandExecutor_->onCommand(sender, command, args);
}

void PlayerRegisterPlugin::onPlayerJoin(endstone::PlayerJoinEvent &event) {
    auto& player = event.getPlayer();
    getLogger().info("Player joined: {}", player.getName());
    
    // Save player's original data
    savePlayerData(player);
    
    // Apply authentication effects
    applyAuthEffects(player);
    
    // Teleport to auth position
    teleportToAuthPosition(player);
    
    // Send title message
    player.sendTitle("Пожалуйста, зарегистрируйтесь для продолжения игры.", "", 20, 600, 20);
    
    // Start auth timer
    startAuthTimer(player);
    
    // Send welcome messages
    player.sendMessage(endstone::ColorFormat::Yellow + "Добро пожаловать на сервер!");
    player.sendMessage(endstone::ColorFormat::Gold + "Используйте /register <пароль> <повтор> для регистрации");
    player.sendMessage(endstone::ColorFormat::Gold + "Или /login <пароль> для входа в существующий аккаунт");
}

void PlayerRegisterPlugin::onPlayerQuit(endstone::PlayerQuitEvent &event) {
    auto& player = event.getPlayer();
    getLogger().info("Player quit: {}", player.getName());
    
    // Stop auth timers
    stopAuthTimer(player);
    
    // Clean up player data
    playerDataMap_.erase(&player);
}

void PlayerRegisterPlugin::onPlayerChat(endstone::PlayerChatEvent &event) {
    auto& player = event.getPlayer();
    
    // If player is not authenticated, cancel the chat event
    if (!isPlayerAuthenticated(&player)) {
        event.setCancelled(true);
        player.sendMessage(endstone::ColorFormat::Red + "Вы должны авторизоваться, чтобы использовать чат!");
    }
}

void PlayerRegisterPlugin::onServerLoad(endstone::ServerLoadEvent &event) {
    getLogger().info("PlayerRegister plugin is ready to handle player authentication!");
}

void PlayerRegisterPlugin::savePlayerData(endstone::Player &player) {
    auto location = player.getLocation();
    playerDataMap_[&player] = PlayerData(location, player.getYaw(), player.getPitch());
}

void PlayerRegisterPlugin::applyAuthEffects(endstone::Player &player) {
    // Apply effects at level 255 (amplifier 254) with hidden particles
    // Note: Effect types may need to be adjusted based on EndStone API
    
    // Blindness effect
    player.addEffect(endstone::EffectType::Blindness, EFFECT_DURATION, EFFECT_AMPLIFIER, false);
    
    // Slow falling effect  
    player.addEffect(endstone::EffectType::SlowFalling, EFFECT_DURATION, EFFECT_AMPLIFIER, false);
    
    // Invisibility effect
    player.addEffect(endstone::EffectType::Invisibility, EFFECT_DURATION, EFFECT_AMPLIFIER, false);
}

void PlayerRegisterPlugin::teleportToAuthPosition(endstone::Player &player) {
    endstone::Location authLocation(20000.0f, 40000.0f, 30000.0f);
    player.teleport(authLocation);
}

void PlayerRegisterPlugin::restorePlayerState(endstone::Player &player) {
    auto it = playerDataMap_.find(&player);
    if (it != playerDataMap_.end()) {
        auto& data = it->second;
        
        // Remove all effects
        player.removeAllEffects();
        
        // Teleport back to original location
        player.teleport(data.originalLocation);
        
        // Note: Rotation restoration might need additional API calls
        // player.setYaw(data.originalYaw);
        // player.setPitch(data.originalPitch);
    }
}

void PlayerRegisterPlugin::completeAuthentication(endstone::Player &player) {
    // Stop auth timer
    stopAuthTimer(player);
    
    // Set player as authenticated
    setPlayerAuthenticated(player, true);
    
    // Restore player state
    restorePlayerState(player);
    
    // Send welcome message
    player.sendMessage(endstone::ColorFormat::Green + "Авторизация успешна! Добро пожаловать на сервер!");
    
    // Clear title
    player.sendTitle("", "", 0, 0, 0);
}

void PlayerRegisterPlugin::startAuthTimer(endstone::Player &player) {
    stopAuthTimer(&player);
    
    auto it = playerDataMap_.find(&player);
    if (it == playerDataMap_.end()) return;
    
    auto& data = it->second;
    
    // Create kick timer
    data.authTimerTask = getServer().getScheduler().runTaskLater(
        *this,
        [this, &player]() {
            kickUnauthedPlayer(player);
        },
        AUTH_TIMEOUT.count() * 20 // Convert to ticks
    );
    
    // Create reminder timer
    data.reminderTask = getServer().getScheduler().runTaskTimer(
        *this,
        [this, &player]() {
            sendAuthReminder(player);
        },
        REMINDER_INTERVAL.count() * 20, // Initial delay
        REMINDER_INTERVAL.count() * 20  // Repeat interval
    );
}

void PlayerRegisterPlugin::stopAuthTimer(endstone::Player &player) {
    auto it = playerDataMap_.find(&player);
    if (it == playerDataMap_.end()) return;
    
    auto& data = it->second;
    
    if (data.authTimerTask) {
        data.authTimerTask->cancel();
        data.authTimerTask.reset();
    }
    
    if (data.reminderTask) {
        data.reminderTask->cancel();
        data.reminderTask.reset();
    }
}

void PlayerRegisterPlugin::sendAuthReminder(endstone::Player &player) {
    if (isPlayerAuthenticated(&player)) {
        stopAuthTimer(player);
        return;
    }
    
    auto it = playerDataMap_.find(&player);
    if (it == playerDataMap_.end()) return;
    
    auto& data = it->second;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - data.joinTime;
    auto remaining = AUTH_TIMEOUT - elapsed;
    
    if (remaining.count() <= 0) {
        kickUnauthedPlayer(player);
        return;
    }
    
    auto secondsLeft = std::chrono::duration_cast<std::chrono::seconds>(remaining).count();
    
    // Send reminder only at specific intervals: 150, 120, 90, 60, 30
    if (secondsLeft == 150 || secondsLeft == 120 || secondsLeft == 90 || secondsLeft == 60 || secondsLeft == 30) {
        // Send chat message
        std::ostringstream chatMsg;
        chatMsg << endstone::ColorFormat::Yellow << "[Auth] Осталось " << secondsLeft 
                << " секунд для авторизации";
        player.sendMessage(chatMsg.str());
        
        // Send title and subtitle
        player.sendTitle("Время авторизации истекает!", 
                        "Осталось: " + std::to_string(secondsLeft) + " секунд", 
                        10, 70, 10);
    }
}

void PlayerRegisterPlugin::kickUnauthedPlayer(endstone::Player &player) {
    player.kick(endstone::ColorFormat::Red + "Время авторизации истекло");
    stopAuthTimer(player);
    playerDataMap_.erase(&player);
}

bool PlayerRegisterPlugin::isPlayerAuthenticated(endstone::Player *player) {
    auto it = playerDataMap_.find(player);
    return it != playerDataMap_.end() && it->second.isAuthenticated;
}

void PlayerRegisterPlugin::setPlayerAuthenticated(endstone::Player &player, bool authenticated) {
    auto it = playerDataMap_.find(&player);
    if (it != playerDataMap_.end()) {
        it->second.isAuthenticated = authenticated;
    }
}

bool PlayerRegisterPlugin::shouldAllowChat(endstone::Player &player) {
    return isPlayerAuthenticated(&player);
}

bool PlayerRegisterPlugin::shouldAllowCommand(endstone::Player &player, const std::string &command) {
    // Always allow register and login commands for unauthenticated players
    if (!isPlayerAuthenticated(&player)) {
        return command == "register" || command == "login";
    }
    return true;
}