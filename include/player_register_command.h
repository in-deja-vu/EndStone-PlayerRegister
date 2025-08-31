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
            sender.sendErrorMessage("Passwords do not match!");
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
            sender.sendErrorMessage("New passwords do not match!");
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
            sender.sendMessage(endstone::ColorFormat::Yellow + "Account Management Commands:");
            sender.sendMessage(endstone::ColorFormat::Gold + "/account - Show account information");
            sender.sendMessage(endstone::ColorFormat::Gold + "/register <username> <password> <confirm> - Create account");
            sender.sendMessage(endstone::ColorFormat::Gold + "/login <username> <password> - Login to account");
            sender.sendMessage(endstone::ColorFormat::Gold + "/changepassword <old> <new> <confirm> - Change password");
            sender.sendMessage(endstone::ColorFormat::Gold + "/logout - Logout from account");
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
            sender.sendErrorMessage("Usage: /resetpassword <username>");
            return true;
        }

        const std::string& username = args[0];
        
        // Generate a random password
        std::string newPassword = std::to_string(rand() % 900000 + 100000); // 6-digit number
        
        if (PlayerRegister::AccountManager::changePassword(username, newPassword)) {
            sender.sendMessage(endstone::ColorFormat::Green + "Password for account '" + username + "' has been reset to: " + newPassword);
            return true;
        } else {
            sender.sendErrorMessage("Account '" + username + "' not found!");
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
            sender.sendMessage(endstone::ColorFormat::Green + "Successfully logged out!");
            PlayerRegister::PlayerManager::reconnect(player);
        } else {
            sender.sendErrorMessage("You are not logged in to any account!");
        }

        return true;
    }
};
