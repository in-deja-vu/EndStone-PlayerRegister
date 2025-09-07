// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "account_manager.h"

#include "sha256.h"
#include "config.h"
#include "database.h"
#include <endstone/endstone.hpp>
#include <algorithm>
#include <random>
#include <sstream>

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
    std::string trimmedPassword = password;
    trimString(trimmedPassword);

    if (!validatePassword(trimmedPassword)) {
        pl.sendMessage(endstone::ColorFormat::Red + "Пароль должен быть не менее 4 символов!");
        return false;
    }

    // Check if account with player name already exists
    PlayerData data;
    data.id = PlayerManager::getId(&pl);
    data.name = pl.getName(); // Use player's actual name instead of provided name
    Database::loadAsAccount(data);
    
    if (data.valid) {
        pl.sendMessage(endstone::ColorFormat::Red + "Аккаунт с таким никнеймом (" + pl.getName() + ") уже существует.");
        return false;
    }

    data.accounts = PlayerManager::getPlayerData(&pl).accounts + 1;
    
    // Check max accounts limit from config
    const int max_accounts = Config::getInstance().max_accounts;
    if (data.accounts > max_accounts) {
        pl.sendMessage(endstone::ColorFormat::Red + "Вы уже создали максимальное количество аккаунтов (" + std::to_string(max_accounts) + ")!");
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
    data.isRegistered = true;
    data.isAuthenticated = true;
    Database::storeAsAccount(data);
    Database::storeAsPlayer(data);
    
    pl.sendMessage(endstone::ColorFormat::Green + "Аккаунт успешно создан!");
    
    // Complete authorization process - this will teleport player back
    PlayerManager::completeAuthorizationProcess(&pl);
    
    // Update player data to mark as authenticated
    PlayerManager::setPlayerData(&pl, data);
    
    // No need for reconnect since we're already authenticated
    
    return true;
}

bool AccountManager::loginAccount(endstone::Player& pl, const std::string& name, const std::string& password) {
    std::string trimmedPassword = password;
    trimString(trimmedPassword);

    PlayerData data;
    data.id = PlayerManager::getId(&pl);
    data.name = pl.getName(); // Use player's actual name
    Database::loadAsAccount(data);

    if (!data.valid) {
        pl.sendMessage(endstone::ColorFormat::Red + "Аккаунт не найден!");
        return false;
    }

    // Hash the provided password and compare with stored hash
    std::string hashedPassword = SHA256::digest_str(trimmedPassword);
    if (data.password != hashedPassword) {
        pl.sendMessage(endstone::ColorFormat::Red + "Неверный пароль!");
        return false;
    }

    data.isRegistered = true;
    data.isAuthenticated = true;
    Database::storeAsPlayer(data);
    PlayerManager::setPlayerData(&pl, data);
    
    pl.sendMessage(endstone::ColorFormat::Green + "Успешный вход в систему!");
    
    // Complete authorization process - this will teleport player back
    PlayerManager::completeAuthorizationProcess(&pl);
    
    // No need for reconnect since we're already authenticated
    
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

    PlayerData data;
    data.name = trimmedName;
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
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Регистрация аккаунта ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Использование: /register <пароль> <подтверждение_пароля>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Требования:");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Пароль: не менее 4 символов");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Пароль и подтверждение должны совпадать");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Аккаунт будет создан на ваш текущий никнейм");
}

void AccountManager::showLoginHelp(endstone::Player& pl) {
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Вход в аккаунт ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Использование: /login <пароль>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Введите пароль для входа в ваш аккаунт.");
    pl.sendMessage(endstone::ColorFormat::Gray + "Аккаунт привязан к вашему текущему никнейму.");
}

void AccountManager::showAccountInfo(endstone::Player& pl) {
    const PlayerData& data = PlayerManager::getPlayerData(&pl);
    
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Информация об аккаунте ===");
    
    if (data.valid && data.accounts > 0) {
        pl.sendMessage(endstone::ColorFormat::Green + "Вы вошли как: " + data.name);
        pl.sendMessage(endstone::ColorFormat::Gray + "Создано аккаунтов: " + std::to_string(data.accounts));
        pl.sendMessage(endstone::ColorFormat::Gold + "Используйте /changepassword для смены пароля");
        pl.sendMessage(endstone::ColorFormat::Gold + "Используйте /logout для выхода из аккаунта");
    } else {
        pl.sendMessage(endstone::ColorFormat::Red + "Вы не вошли в аккаунт!");
        pl.sendMessage(endstone::ColorFormat::Gold + "Используйте /register для создания аккаунта");
        pl.sendMessage(endstone::ColorFormat::Gold + "Используйте /login для входа в существующий аккаунт");
    }
}

void AccountManager::showChangePasswordHelp(endstone::Player& pl) {
    pl.sendMessage(endstone::ColorFormat::Yellow + "=== Смена пароля ===");
    pl.sendMessage(endstone::ColorFormat::Gold + "Использование: /changepassword <старый_пароль> <новый_пароль> <подтверждение_нового_пароля>");
    pl.sendMessage(endstone::ColorFormat::Gray + "Требования:");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Вы должны быть вошли в аккаунт");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Старый пароль должен быть верным");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Новый пароль должен быть не менее 4 символов");
    pl.sendMessage(endstone::ColorFormat::Gray + "  • Новый пароль и подтверждение должны совпадать");
}

} // namespace PlayerRegister