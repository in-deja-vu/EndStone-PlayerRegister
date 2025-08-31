// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include "player_register_listener.h"
#include "player_register_command.h"
#include "config.h"
#include "database.h"

#include <endstone/endstone.hpp>
#include <memory>
#include <vector>

class PlayerRegisterPlugin : public endstone::Plugin {
public:
    void onLoad() override
    {
        getLogger().info("PlayerRegister plugin loading...");
        
        // Initialize configuration
        if (!PlayerRegister::Config::init(getDataFolder())) {
            getLogger().error("Failed to initialize configuration!");
            return;
        }
        
        // Initialize database
        if (!PlayerRegister::Database::init(getDataFolder())) {
            getLogger().error("Failed to initialize database!");
            return;
        }
        
        getLogger().info("Configuration and database initialized successfully.");
    }

    void onEnable() override
    {
        getLogger().info("PlayerRegister plugin enabled!");

        // Set up command executors for all commands
        if (auto *command = getCommand("register")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        if (auto *command = getCommand("login")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        if (auto *command = getCommand("changepassword")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        if (auto *command = getCommand("account")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        if (auto *command = getCommand("resetpassword")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        if (auto *command = getCommand("logout")) {
            command->setExecutor(std::make_unique<PlayerRegisterCommandExecutor>());
        }

        // Register event handlers
        registerEvent(&PlayerRegisterPlugin::onPlayerJoin, *this);
        registerEvent(&PlayerRegisterPlugin::onPlayerQuit, *this);
        registerEvent(&PlayerRegisterPlugin::onServerLoad, *this);

        // Register listener for additional events
        listener_ = std::make_unique<PlayerRegisterListener>(*this);
        registerEvent(&PlayerRegisterListener::onServerLoad, *listener_, endstone::EventPriority::High);
        
        getLogger().info("All commands and events registered successfully!");
    }

    void onDisable() override
    {
        getLogger().info("PlayerRegister plugin disabled!");
        
        // Clean up player data
        PlayerRegister::PlayerManager::getAllData().clear();
    }

    // Event handlers
    void onPlayerJoin(endstone::PlayerJoinEvent &event)
    {
        auto* player = event.getPlayer();
        getLogger().info("Player joined: {}", player->getName());
        
        // Load player data when they join
        PlayerRegister::PlayerManager::loadPlayer(player);
        
        // Check if player needs to login (not authenticated)
        const PlayerRegister::PlayerData& data = PlayerRegister::PlayerManager::getPlayerData(player);
        if (!data.valid) {
            player->sendMessage(endstone::ColorFormat::Yellow + "Please login or register to play!");
            player->sendMessage(endstone::ColorFormat::Gold + "Use /register <username> <password> <confirm> to create an account");
            player->sendMessage(endstone::ColorFormat::Gold + "Use /login <username> <password> to login to an existing account");
        }
    }

    void onPlayerQuit(endstone::PlayerQuitEvent &event)
    {
        auto* player = event.getPlayer();
        getLogger().info("Player quit: {}", player->getName());
        
        // Clean up player data when they leave
        PlayerRegister::PlayerManager::unloadPlayer(player);
    }

    void onServerLoad(endstone::ServerLoadEvent &event)
    {
        getLogger().info("{} is passed to PlayerRegisterPlugin::onServerLoad", event.getEventName());
        getLogger().info("PlayerRegister plugin is ready to handle account registrations!");
    }

private:
    std::unique_ptr<PlayerRegisterListener> listener_;
};
