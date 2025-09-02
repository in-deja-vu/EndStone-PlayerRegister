// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "account_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace PlayerRegister {

std::string AccountManager::dataDir_;

bool AccountManager::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    ensureDirectoryExists(dataDir_);
    return true;
}

bool AccountManager::createAccount(const std::string& username, const std::string& password) {
    if (!isValidPassword(password)) {
        return false;
    }
    
    if (accountExists(username)) {
        return false; // Account already exists
    }
    
    std::string hashedPassword = hashPassword(password);
    return saveAccountData(username, hashedPassword);
}

bool AccountManager::accountExists(const std::string& username) {
    auto accountData = loadAccountData(username);
    return accountData.has_value();
}

bool AccountManager::verifyPassword(const std::string& username, const std::string& password) {
    auto accountData = loadAccountData(username);
    if (!accountData) {
        return false; // Account doesn't exist
    }
    
    std::string inputHash = hashPassword(password);
    return inputHash == *accountData;
}

bool AccountManager::removeAccount(const std::string& username) {
    std::string filePath = getAccountFilePath(username);
    if (std::filesystem::exists(filePath)) {
        return std::filesystem::remove(filePath);
    }
    return false;
}

std::string AccountManager::hashPassword(const std::string& password) {
    // Simple hash function for demonstration
    // In production, use proper cryptographic hashing like bcrypt or Argon2
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool AccountManager::isValidPassword(const std::string& password) {
    if (password.empty() || password.length() < 4 || password.length() > 32) {
        return false;
    }
    return true;
}

void AccountManager::sendRegisterError(endstone::Player& player, const std::string& error) {
    player.sendMessage(endstone::ColorFormat::Red + "Ошибка регистрации: " + error);
}

void AccountManager::sendLoginError(endstone::Player& player, const std::string& error) {
    player.sendMessage(endstone::ColorFormat::Red + "Ошибка входа: " + error);
}

void AccountManager::sendSuccessMessage(endstone::Player& player, const std::string& message) {
    player.sendMessage(endstone::ColorFormat::Green + message);
}

std::string AccountManager::getAccountFilePath(const std::string& username) {
    return dataDir_ + "/accounts/" + username + ".json";
}

void AccountManager::ensureDirectoryExists(const std::string& path) {
    std::filesystem::create_directories(path + "/accounts");
}

bool AccountManager::saveAccountData(const std::string& username, const std::string& hashedPassword) {
    std::string filePath = getAccountFilePath(username);
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << "{\n";
        file << "  \"username\": \"" << username << "\",\n";
        file << "  \"password_hash\": \"" << hashedPassword << "\",\n";
        file << "  \"created_at\": \"" << std::time(nullptr) << "\"\n";
        file << "}";
        
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> AccountManager::loadAccountData(const std::string& username) {
    std::string filePath = getAccountFilePath(username);
    
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return std::nullopt;
        }
        
        std::string line;
        std::string passwordHash;
        
        while (std::getline(file, line)) {
            if (line.find("\"password_hash\"") != std::string::npos) {
                size_t start = line.find(": \"") + 3;
                size_t end = line.find("\"", start);
                if (start != std::string::npos && end != std::string::npos) {
                    passwordHash = line.substr(start, end - start);
                    break;
                }
            }
        }
        
        file.close();
        
        if (passwordHash.empty()) {
            return std::nullopt;
        }
        
        return passwordHash;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace PlayerRegister