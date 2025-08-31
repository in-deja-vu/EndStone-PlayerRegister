// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#pragma once

#include <endstone/endstone.hpp>
#include <string>
#include "account_manager.h"
#include "database.h"

class PlayerRegisterCommandExecutor : public endstone::CommandExecutor {
public:
    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command,
                   const std::vector<std::string> &args) override
    {
        if (!command.testPermission(sender)) {
            return true;
        }

        if (command.getName() == "register") {
            return handleRegister(sender, args);
        }

        if (command.getName() == "login") {
            return handleLogin(sender, args);
        }

        if (command.getName() == "changepassword") {
            return handleChangePassword(sender, args);
        }

        if (command.getName() == "account") {
            return handleAccount(sender, args);
        }

        if (command.getName() == "resetpassword") {
            return handleResetPassword(sender, args);
        }

        if (command.getName() == "logout") {
            return handleLogout(sender, args);
        }

        return false;
    }

private:
    bool handleRegister(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        auto* player = sender.asPlayer();
        if (!player) {
            sender.sendErrorMessage("This command can only be used by players!");
            return true;
        }

        if (args.size() < 3) {
            PlayerRegister::AccountManager::showRegisterHelp(*player);
            return true;
        }

        const std::string& username = args[0];
        const std::string& password = args[1];
        const std::string& confirmPassword = args[2];

        if (password != confirmPassword) {
            sender.sendErrorMessage("Пароли не совпадают!");
            return true;
        }

        return PlayerRegister::AccountManager::createAccount(*player, username, password);
    }

    bool handleLogin(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        auto* player = sender.asPlayer();
        if (!player) {
            sender.sendErrorMessage("This command can only be used by players!");
            return true;
        }

        if (args.size() < 2) {
            PlayerRegister::AccountManager::showLoginHelp(*player);
            return true;
        }

        const std::string& username = args[0];
        const std::string& password = args[1];

        return PlayerRegister::AccountManager::loginAccount(*player, username, password);
    }

    bool handleChangePassword(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        auto* player = sender.asPlayer();
        if (!player) {
            sender.sendErrorMessage("This command can only be used by players!");
            return true;
        }

        if (args.size() < 3) {
            PlayerRegister::AccountManager::showChangePasswordHelp(*player);
            return true;
        }

        const std::string& oldPassword = args[0];
        const std::string& newPassword = args[1];
        const std::string& confirmPassword = args[2];

        if (newPassword != confirmPassword) {
            sender.sendErrorMessage("Новые пароли не совпадают!");
            return true;
        }

        return PlayerRegister::AccountManager::changePassword(*player, oldPassword, newPassword);
    }

    bool handleAccount(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        auto* player = sender.asPlayer();
        if (!player) {
            sender.sendErrorMessage("This command can only be used by players!");
            return true;
        }

        if (args.empty()) {
            PlayerRegister::AccountManager::showAccountInfo(*player);
            return true;
        }

        const std::string& action = args[0];
        
        if (action == "info") {
            PlayerRegister::AccountManager::showAccountInfo(*player);
        } else {
            sender.sendMessage(endstone::ColorFormat::Yellow + "Команды управления аккаунтом:");
            sender.sendMessage(endstone::ColorFormat::Gold + "/account - Показать информацию об аккаунте");
            sender.sendMessage(endstone::ColorFormat::Gold + "/register <ник> <пароль> <подтверждение> - Создать аккаунт");
            sender.sendMessage(endstone::ColorFormat::Gold + "/login <ник> <пароль> - Войти в аккаунт");
            sender.sendMessage(endstone::ColorFormat::Gold + "/changepassword <старый> <новый> <подтверждение> - Сменить пароль");
            sender.sendMessage(endstone::ColorFormat::Gold + "/logout - Выйти из аккаунта");
        }

        return true;
    }

    bool handleResetPassword(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        // Check if sender has operator permissions
        if (!sender.hasPermission("endstone.command.op")) {
            sender.sendErrorMessage("This command can only be used by operators!");
            return true;
        }

        if (args.size() < 1) {
            sender.sendErrorMessage("Использование: /resetpassword <ник>");
            return true;
        }

        const std::string& username = args[0];
        
        // Generate a random password
        std::string newPassword = std::to_string(rand() % 900000 + 100000); // 6-digit number
        
        if (PlayerRegister::AccountManager::changePassword(username, newPassword)) {
            sender.sendMessage(endstone::ColorFormat::Green + "Пароль для аккаунта '" + username + "' был сброшен на: " + newPassword);
            return true;
        } else {
            sender.sendErrorMessage("Аккаунт '" + username + "' не найден!");
            return true;
        }
    }

    bool handleLogout(endstone::CommandSender &sender, const std::vector<std::string> &args)
    {
        auto* player = sender.asPlayer();
        if (!player) {
            sender.sendErrorMessage("This command can only be used by players!");
            return true;
        }
        
        // Remove player data to logout
        if (PlayerRegister::Database::removePlayer(PlayerRegister::PlayerManager::getId(player))) {
            sender.sendMessage(endstone::ColorFormat::Green + "Успешный выход из аккаунта!");
            PlayerRegister::PlayerManager::reconnect(player);
        } else {
            sender.sendErrorMessage("Вы не вошли в аккаунт!");
        }

        return true;
    }
};
