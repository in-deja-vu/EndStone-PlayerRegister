// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "auth_command_executor.h"
#include "player_register_plugin.h"
#include "account_manager.h"

AuthCommandExecutor::AuthCommandExecutor(PlayerRegisterPlugin& plugin) : plugin_(plugin) {}

bool AuthCommandExecutor::onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                                   const std::vector<std::string> &args) {
    if (!command.testPermission(sender)) {
        return true;
    }

    // Check if sender is a player and not authenticated
    auto* player = sender.asPlayer();
    if (player && !plugin_.isPlayerAuthenticated(player)) {
        // Only allow register and login commands for unauthenticated players
        if (command.getName() != "register" && command.getName() != "login") {
            player->sendMessage(endstone::ColorFormat::Red + "Вы должны авторизоваться, чтобы использовать команды!");
            return true;
        }
    }

    if (command.getName() == "register") {
        return handleRegister(sender, args);
    }

    if (command.getName() == "login") {
        return handleLogin(sender, args);
    }

    return false;
}

bool AuthCommandExecutor::handleRegister(endstone::CommandSender &sender, const std::vector<std::string> &args) {
    auto* player = sender.asPlayer();
    if (!player) {
        sender.sendErrorMessage("This command can only be used by players!");
        return true;
    }

    // Check if player is already authenticated
    if (plugin_.isPlayerAuthenticated(player)) {
        player->sendMessage(endstone::ColorFormat::Red + "Вы уже авторизованы!");
        return true;
    }

    if (args.size() < 2) {
        player->sendMessage(endstone::ColorFormat::Red + "Использование: /register <пароль> <повтор>");
        return true;
    }

    const std::string& password = args[0];
    const std::string& confirmPassword = args[1];

    // Check if passwords match
    if (password != confirmPassword) {
        PlayerRegister::AccountManager::sendRegisterError(*player, "Пароли не совпадают!");
        return true;
    }

    // Check if account already exists
    std::string username = player->getName();
    if (PlayerRegister::AccountManager::accountExists(username)) {
        PlayerRegister::AccountManager::sendRegisterError(*player, 
            "Аккаунт с таким никнеймом (" + username + ") уже существует.");
        return true;
    }

    // Create account
    if (PlayerRegister::AccountManager::createAccount(username, password)) {
        PlayerRegister::AccountManager::sendSuccessMessage(*player, "Регистрация успешна!");
        
        // Complete authentication
        plugin_.completeAuthentication(*player);
    } else {
        PlayerRegister::AccountManager::sendRegisterError(*player, "Не удалось создать аккаунт.");
    }

    return true;
}

bool AuthCommandExecutor::handleLogin(endstone::CommandSender &sender, const std::vector<std::string> &args) {
    auto* player = sender.asPlayer();
    if (!player) {
        sender.sendErrorMessage("This command can only be used by players!");
        return true;
    }

    // Check if player is already authenticated
    if (plugin_.isPlayerAuthenticated(player)) {
        player->sendMessage(endstone::ColorFormat::Red + "Вы уже авторизованы!");
        return true;
    }

    if (args.size() < 1) {
        player->sendMessage(endstone::ColorFormat::Red + "Использование: /login <пароль>");
        return true;
    }

    const std::string& password = args[0];
    std::string username = player->getName();

    // Check if account exists
    if (!PlayerRegister::AccountManager::accountExists(username)) {
        PlayerRegister::AccountManager::sendLoginError(*player, "Аккаунт не найден. Используйте /register для создания.");
        return true;
    }

    // Verify password
    if (PlayerRegister::AccountManager::verifyPassword(username, password)) {
        // Complete authentication
        plugin_.completeAuthentication(*player);
    } else {
        PlayerRegister::AccountManager::sendLoginError(*player, "Неверный пароль!");
    }

    return true;
}