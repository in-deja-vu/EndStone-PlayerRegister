// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "player_register_listener.h"
#include "player_manager.h"
#include <endstone/endstone.hpp>

void PlayerRegisterListener::onServerLoad(endstone::ServerLoadEvent &event)
{
    plugin_.getLogger().info("ServerLoadEvent is passed to PlayerRegisterListener::onServerLoad");
}

void PlayerRegisterListener::onPlayerJoin(endstone::PlayerJoinEvent &event)
{
    auto& player = event.getPlayer();
    plugin_.getLogger().info("Player joined: {}", player.getName());
    
    // Start authorization process for the player
    PlayerRegister::PlayerManager::startAuthorizationProcess(&player);
}

void PlayerRegisterListener::onPlayerQuit(endstone::PlayerQuitEvent &event)
{
    auto& player = event.getPlayer();
    plugin_.getLogger().info("Player quit: {}", player.getName());
    
    // Clean up player data
    PlayerRegister::PlayerManager::unloadPlayer(&player);
}

void PlayerRegisterListener::onPlayerChat(endstone::PlayerChatEvent &event)
{
    auto& player = event.getPlayer();
    
    // Check if player is authorized
    if (!PlayerRegister::PlayerManager::isPlayerAuthorized(&player)) {
        // Player is not authorized, cancel the chat event
        event.setCancelled(true);
        player.sendMessage(endstone::ColorFormat::Red + "Вы должны авторизоваться, чтобы писать в чат!");
        player.sendMessage(endstone::ColorFormat::Gold + "Используйте /register <пароль> <подтверждение> или /login <пароль>");
    }
}

void PlayerRegisterListener::onPlayerCommand(endstone::PlayerCommandEvent &event)
{
    auto& player = event.getPlayer();
    std::string command = event.getCommand();
    
    // Remove the leading slash
    if (!command.empty() && command[0] == '/') {
        command = command.substr(1);
    }
    
    // Get the command name (first word)
    size_t space_pos = command.find(' ');
    std::string command_name = command.substr(0, space_pos);
    
    // Check if player is authorized
    if (!PlayerRegister::PlayerManager::isPlayerAuthorized(&player)) {
        // Player is not authorized, check if command is allowed
        if (!PlayerRegister::PlayerManager::isCommandAllowed(command_name)) {
            // Command is not allowed, cancel the event
            event.setCancelled(true);
            player.sendMessage(endstone::ColorFormat::Red + "Вы должны авторизоваться, чтобы использовать команды!");
            player.sendMessage(endstone::ColorFormat::Gold + "Используйте /register <пароль> <подтверждение> или /login <пароль>");
        }
    }
}