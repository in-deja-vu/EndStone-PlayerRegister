// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "account_manager.h"

#include "sha256.h"
#include "config.h"
#include <endstone/endstone.hpp>
#include <algorithm>
#include <random>
#include <sstream>

// Forward declarations for database functions
namespace PlayerRegister {
namespace Database {
    void storeAsPlayer(const PlayerData& data);
    void loadAsPlayer(PlayerData& data);
    bool removePlayer(const std::string& id);
    void storeAsAccount(const PlayerData& data);
    void loadAsAccount(PlayerData& data);
}
}

namespace PlayerRegister {

void AccountManager::trimString(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

bool AccountManager::validatePassword(const std::string& password) {
    return password.length() >= 4;
}

bool AccountManager::validateUsername(const std::string& username) {
    return username.length() >= 3 && username.length() <= 16;
}

bool AccountManager::createAccount(endstone::Player& pl, const std::string& name, const std::string& password, bool create_new) {
    std::string trimmedName = name;
    std::string trimmedPassword = password;
    trimString(trimmedName);
    trimString(trimmedPassword);

    if (!validateUsername(trimmedName)) {
        pl.sendMessage(endstone::ColorFormat::Red + "Username must be between 3 and 16 characters long!");
        return false;
    }

    if (!validatePassword(trimmedPassword)) {
        pl.sendMessage(endstone::ColorFormat::Red + "Password must be at least 4 characters long!");
        return false;
    }

    PlayerData data(PlayerManager::getId(&pl), trimmedName);
    Database::loadAsAccount(data);
    
    if (data.valid) {
        pl.sendMessage(endstone::ColorFormat::Red + "An account with this name already exists!");
        return false;
    }

    data.accounts = PlayerManager::getPlayerData(&pl).accounts + 1;
    
    // Check max accounts limit from config
    const int max_accounts = Config::getInstance().max_accounts;
    if (data.accounts > max_accounts) {
        pl.sendMessage(endstone::ColorFormat::Red + "You have already created the maximum number of accounts (" + std::to_string(max_accounts) + ")!");
        return false;
    }

    // Hash the password using SHA256
    data.password = SHA256::digest_str(trimmedPassword);
    
    if (create_new) {
        // Generate a random UUID using a simple method
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        uint64_t high = dis(gen);
        uint64_t low = dis(gen);
        
        // Create UUID by filling the byte array
        endstone::UUID uuid;
        for (int i = 0; i < 8; i++) {
            uuid.data[i] = (high >> (8 * (7 - i))) & 0xFF;
            uuid.data[8 + i] = (low >> (8 * (7 - i))) & 0xFF;
        }
        data.fakeUUID = uuid;
        data.fakeDBkey = "player_server_" + data.fakeUUID.str();
    } else {
        data.fakeUUID = PlayerManager::getFakeUUID(&pl);
        // Endstone doesn't have XUID like LeviLamina, so we'll use a placeholder
        data.fakeXUID = pl.getUniqueId().str();
        data.fakeDBkey = PlayerManager::getFakeDBkey(pl.getUniqueId().str());
    }

    if (data.fakeXUID.empty()) {
        data.fakeXUID = std::to_string(rand()); // Simple random number as placeholder
    }

    data.valid = true;
    Database::storeAsAccount(data);
    Database::storeAsPlayer(data);
    
    pl.sendMessage(endstone::ColorFormat::Green + "Account created successfully! You can now login with /login <username> <password>");
    
    if (create_new) {
        PlayerManager::reconnect(&pl);
    } else {
        PlayerManager::setPlayerData(&pl, data);
    }
    
    return true;
}

bool AccountManager::loginAccount(endstone::Player& pl, const std::string& name, const std::string& password) {
    std::string trimmedName = name;
    std::string trimmedPassword = password;
    trimString(trimmedName);
    trimString(trimmedPassword);

    PlayerData data(PlayerManager::getId(&pl), trimmedName);
    Database::loadAsAccount(data);

    if (!data.valid) {
        pl.sendMessage(endstone::ColorFormat::Red + "Account not found!");
        return false;
    }

    // Hash the provided password and compare with stored hash
    std::string hashedPassword = SHA256::digest_str(trimmedPassword);
    if (data.password != hashedPassword) {
        pl.sendMessage(endstone::ColorFormat::Red + "Incorrect password!");
        return false;
    }

    Database::storeAsPlayer(data);
    PlayerManager::setPlayerData(&pl, data);
    
    pl.sendMessage(endstone::ColorFormat::Green + "Successfully logged in!");
    PlayerManager::reconnect(&pl);
    
    return true;
}

bool AccountManager::changePassword(const std::string& name, const std::string& new_password) {
    std::string trimmedName = name;
    std::string trimmedNewPassword = new_password;
    trimString(trimmedName);
    trimString(trimmedNewPassword);

    if (!validatePassword(trimmedNewPassword)) {
        return false;
    }

    PlayerData data{.name = trimmedName};
    Database::loadAsAccount(data);

    if (!data.valid) {
        return false;
    }

    // Hash the new password
    data.password = SHA256::digest_str(trimmedNewPassword);
    Database::storeAsAccount(data);
    
    return true;
}

bool AccountManager::changePassword(endstone::Player& pl, const std::string& old_password, const std::string& new_password) {
    std::string trimmedOldPassword = old_password;
    std::string trimmedNewPassword = new_password;
    trimString(trimmedOldPassword);
    trimString(trimmedNewPassword);

    if (!validatePassword(trimmedNewPassword)) {
        pl.sendMessage(endstone::ColorFormat::Red + "New password must be at least 4 characters long!");
        return false;
    }

    const PlayerData& currentData = PlayerManager::getPlayerData(&pl);
    if (!currentData.valid) {
        pl.sendMessage(endstone::ColorFormat::Red + "You are not logged in to an account!");
        return false;
    }

    // Hash the old password and compare with stored hash
    std::string hashedOldPassword = SHA256::digest_str(trimmedOldPassword);
    if (currentData.password != hashedOldPassword) {
        pl.sendMessage(endstone::ColorFormat::Red + "Incorrect old password!");
        return false;
    }

    PlayerData data = currentData;
    // Hash the new password
    data.password = SHA256::digest_str(trimmedNewPassword);
    Database::storeAsAccount(data);
    
    pl.sendMessage(endstone::ColorFormat::Green + "Password changed successfully!");
    return true;
}

void AccountManager::showRegisterHelp(endstone::Player& pl) {
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Account Registration ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Usage: /register <username> <password> <confirm_password>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Requirements:");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Username: 3-16 characters");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Password: at least 4 characters");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Password and confirmation must match");
}

void AccountManager::showLoginHelp(endstone::Player& pl) {
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Account Login ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Usage: /login <username> <password>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Enter your account username and password to login.");
}

void AccountManager::showAccountInfo(endstone::Player& pl) {
    const PlayerData& data = PlayerManager::getPlayerData(&pl);
    
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Account Information ===");
    
    if (data.valid && data.accounts > 0) {
        pl.sendMessage(endstone::ColorFormat::Green + "Logged in as: " + data.name);
        pl.sendMessage(endstone::ColorFormat::Gray + "Accounts created: " + std::to_string(data.accounts));
        pl.sendMessage(endstone::ColorFormat::Gold + "Use /changepassword to change your password");
        pl.sendMessage(endstone::ColorFormat::Gold + "Use /logout to logout from your account");
    } else {
        pl.sendMessage(endstone::ColorFormat::Red + "You are not logged in to any account!");
        pl.sendMessage(endstone::ColorFormat::Gold + "Use /register to create an account");
        pl.sendMessage(endstone::ColorFormat::Gold + "Use /login to login to an existing account");
    }
}

void AccountManager::showChangePasswordHelp(endstone::Player& pl) {
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Change Password ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Usage: /changepassword <old_password> <new_password> <confirm_new_password>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Requirements:");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Must be logged in to an account");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Old password must be correct");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • New password must be at least 4 characters");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • New password and confirmation must match");
}

} // namespace PlayerRegister